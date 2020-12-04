/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 University of Connecticut
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Robert Martin <robert.martin@engr.uconn.edu>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aqua-sim-ng-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/callback.h"
#include "ns3/flow-monitor-helper.h"

/*
 * BroadCastMAC
 *
 * String topology:
 * N ---->  N  -----> N -----> N* -----> S
 *
 */


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ASBroadcastMac");
void ReceivePacket (Ptr<Socket> socket)
{
 Ptr<Packet> packet;
 while (packet == socket->Recv ())
{
 NS_LOG_INFO("Received packet is size is:" << packet->GetSize () );
}
}

int
main (int argc, char *argv[])
{
  
  double simStop = 100; //seconds
  int nodes = 3;
  int sinks = 1;
  uint32_t m_dataRate = 128;
  uint32_t m_packetSize = 40;
  //double range = 20;

  LogComponentEnable ("ASBroadcastMac", LOG_LEVEL_INFO);

  //to change on the fly
  CommandLine cmd;
  cmd.AddValue ("simStop", "Length of simulation", simStop);
  cmd.AddValue ("nodes", "Amount of regular underwater nodes", nodes);
  cmd.AddValue ("sinks", "Amount of underwater sinks", sinks);
  cmd.Parse(argc,argv);

  std::cout << "-----------Initializing simulation-----------\n";

  NodeContainer nodesCon;
  NodeContainer sinksCon;
  nodesCon.Create(nodes);
  sinksCon.Create(sinks);

  PacketSocketHelper socketHelper;
  socketHelper.Install(nodesCon);
  socketHelper.Install(sinksCon);

  //establish layers using helper's pre-build settings
  AquaSimChannelHelper channel = AquaSimChannelHelper::Default();
  //channel.SetPropagation("ns3::AquaSimRangePropagation");
  AquaSimHelper asHelper = AquaSimHelper::Default();
  asHelper.SetChannel(channel.Create());
  asHelper.SetMac("ns3::AquaSimBroadcastMac");
  asHelper.SetRouting("ns3::AquaSimDBR");

  /*
   * Set up mobility model for nodes and sinks
   */
  MobilityHelper mobility;
  NetDeviceContainer devices;
  Ptr<ListPositionAllocator> position = CreateObject<ListPositionAllocator> ();
  Vector boundry = Vector(0,0,0);

  std::cout << "Creating Nodes\n";

  for (NodeContainer::Iterator i = nodesCon.Begin(); i != nodesCon.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
      position->Add(boundry);
      devices.Add(asHelper.Create(*i, newDevice));

      NS_LOG_DEBUG("Node:" << newDevice->GetAddress () << " position(x):" << boundry.x);
      // NS_LOG_INFO("Interface Addr: " << devices.Get(i)->GetAddress() );
      boundry.x += 10;
      //newDevice->GetPhy()->SetTransRange(range);
    }

  for (NodeContainer::Iterator i = sinksCon.Begin(); i != sinksCon.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
      position->Add(boundry);
      devices.Add(asHelper.Create(*i, newDevice));
      NS_LOG_DEBUG("Sink:" << newDevice->GetAddress () << " position(x):" << boundry.x);
      boundry.x += 10;
      //newDevice->GetPhy()->SetTransRange(range);
    }

  // Print out all interface addresses of nodes

  // for (uint32_t i =0; i < nodesCon.GetN () + 1; i++)
  //{

  // NS_LOG_INFO("The interface address of node " << i << " is " << devices.Get(i)->GetAddress () );

  //}

  mobility.SetPositionAllocator(position);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(nodesCon);
  mobility.Install(sinksCon);

  PacketSocketAddress socket;
  //socket.SetAllDevices();
  socket.SetSingleDevice(0); // Set a specific outgoing physical address
  socket.SetPhysicalAddress (devices.Get(3)->GetAddress()); //Set dest to first sink (nodes+1 device)
  socket.SetProtocol (0);

  OnOffHelper app ("ns3::PacketSocketFactory", Address (socket));
  app.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  app.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  app.SetAttribute ("DataRate", DataRateValue (m_dataRate));
  app.SetAttribute ("PacketSize", UintegerValue (m_packetSize));


  // Echo source is installed on the source nodes 
  ApplicationContainer apps = app.Install (nodesCon);
  apps.Start (Seconds (0.5));
  apps.Stop (Seconds (simStop));


  Ptr<Node> sinkNode = sinksCon.Get(0);
  TypeId psfid = TypeId::LookupByName ("ns3::PacketSocketFactory");

  Ptr<Socket> sinkSocket = Socket::CreateSocket (sinkNode, psfid);
  sinkSocket->Bind (socket);
  sinkSocket->SetRecvCallback (MakeCallback (&ReceivePacket));

   // Installation of Flow Monitor
  Ptr<FlowMonitor> flowmonitor;
  FlowMonitorHelper flowhelper;
  flowmonitor = flowhelper.InstallAll();

  Packet::EnablePrinting (); //for debugging purposes
  std::cout << "-----------Running Simulation-----------\n";
  Simulator::Stop(Seconds(simStop+2));
  Simulator::Run();
  flowmonitor->SerializeToXmlFile("onandoffapp_dbr.xml", true, true);
  Simulator::Destroy();

  return 0;
}
