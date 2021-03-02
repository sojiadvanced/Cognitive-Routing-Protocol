/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Michigan Technological University
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
 * 
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/aqua-sim-ng-module.h"
#include "ns3/applications-module.h"
#include "ns3/log.h"
#include "ns3/callback.h"
#include "ns3/flow-monitor-helper.h"
#include <fstream>
#include "ns3/netanim-module.h"
//#include "ns3/ipv4-global-routing-helper.h"
//#include "ns3/internet-module.h"
/*
 *------------------------------------------------------------
 * Network topology:
 * -----------------------------------------------------------
 *                                                    Sink
 * 
 * 
 *                             N4
 * 
 * 
 *                  N3
 *                                      N7
 *      N1
 *               N5         N6    
 * Src
 *
 *           N2
 * 
 * 
 */


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("OnandOffApp_CARPRouting");

int
main ()
{
  double simStop = 100; //seconds
  int nodes = 3;
 // int carrier_frequency = 24 kHz;
 // int bandwidth = 4 kHz;
  int sinks = 1;
  uint32_t m_dataRate = 16000;
  int m_packetSize = 100;
  std::string asciiTraceFile = "carp_sim_static.asc";

  LogComponentEnable ("OnandOffApp_CARPRouting", LOG_LEVEL_INFO);

  //To change on the fly
  CommandLine cmd;
 // cmd.AddValue ("simStop", "Length of simulation", simStop);
 cmd.AddValue ("nodes", "Amount of regular underwater nodes", nodes);


  std::cout << "-----------Initializing simulation-----------\n";
  std::cout <<"Running the simulation using " << nodes << " nodes\n"; 

  NodeContainer nodesCon;
  NodeContainer sinksCon;
  nodesCon.Create(nodes);
  sinksCon.Create(sinks);

  PacketSocketHelper socketHelper;
  socketHelper.Install(nodesCon);
  socketHelper.Install(sinksCon);

  //Establish layers using helper's pre-build settings
  AquaSimChannelHelper channel = AquaSimChannelHelper::Default();
  channel.SetPropagation("ns3::AquaSimRangePropagation");
  AquaSimHelper asHelper = AquaSimHelper::Default();
  asHelper.SetChannel(channel.Create());
  asHelper.SetMac("ns3::AquaSimSFama");  // Changed this to FAMA
  asHelper.SetRouting("ns3::AquaSimCarp");

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

      //NS_LOG_DEBUG("Node:" << newDevice->GetAddress () << " position(x):" << boundry.x);
      //boundry.x += 10;
    }

  for (NodeContainer::Iterator i = sinksCon.Begin(); i != sinksCon.End(); i++)
    {
      Ptr<AquaSimNetDevice> newDevice = CreateObject<AquaSimNetDevice>();
      position->Add(boundry);
      devices.Add(asHelper.Create(*i, newDevice));
      //NS_LOG_DEBUG("Sink:" << newDevice->GetAddress () << " position(x):" << boundry.x);
      //boundry.x += 10;
    }

  mobility.SetPositionAllocator(position);
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  
  mobility.Install(nodesCon);
  mobility.Install(sinksCon);
  
  Ptr<MobilityModel> nod0 = nodesCon.Get(0)->GetObject<MobilityModel>();
  nod0->SetPosition(boundry);
  
  // Network Topology I
  //Ptr<MobilityModel> nod1 = nodesCon.Get(1)->GetObject<MobilityModel>();
  //nod1->SetPosition(Vector(boundry.x + 10, boundry.y+0, boundry.z));
  //Ptr<MobilityModel> nod2 = nodesCon.Get(2)->GetObject<MobilityModel>();
  //nod2->SetPosition(Vector(boundry.x + 10, boundry.y+20, boundry.z));
  //Ptr<MobilityModel> nod3 = nodesCon.Get(0)->GetObject<MobilityModel>();
  //nod3->SetPosition(Vector(boundry.x + 25, boundry.y+20, boundry.z));
  //Ptr<MobilityModel> nod4 = nodesCon.Get(0)->GetObject<MobilityModel>();
  //nod4->SetPosition(Vector(boundry.x + 35, boundry.y+15, boundry.z));
  //Ptr<MobilityModel> nod5 = nodesCon.Get(0)->GetObject<MobilityModel>();
  //nod5->SetPosition(Vector(boundry.x + 30, boundry.y+40, boundry.z));
  //Ptr<MobilityModel> nod6 = nodesCon.Get(0)->GetObject<MobilityModel>();
  //nod6->SetPosition(Vector(boundry.x + 40, boundry.y+30, boundry.z));
 //Ptr<MobilityModel> nod7 = nodesCon.Get(0)->GetObject<MobilityModel>();
  //nod7->SetPosition(Vector(boundry.x + 45, boundry.y+20, boundry.z));
  
  // Network Topology II
   Ptr<MobilityModel> nod1 = nodesCon.Get(1)->GetObject<MobilityModel>();
  nod1->SetPosition(Vector(boundry.x + 5, boundry.y+10, boundry.z));
  Ptr<MobilityModel> nod2 = nodesCon.Get(2)->GetObject<MobilityModel>();
  nod2->SetPosition(Vector(boundry.x + 15, boundry.y+10, boundry.z));
  //Ptr<MobilityModel> nod3 = nodesCon.Get(0)->GetObject<MobilityModel>();
  //nod3->SetPosition(Vector(boundry.x + 25, boundry.y+5, boundry.z));
  //Ptr<MobilityModel> nod4 = nodesCon.Get(0)->GetObject<MobilityModel>();
  //nod4->SetPosition(Vector(boundry.x + 28, boundry.y+25, boundry.z));
  //Ptr<MobilityModel> nod5 = nodesCon.Get(0)->GetObject<MobilityModel>();
  //nod5->SetPosition(Vector(boundry.x + 45, boundry.y+5, boundry.z));
  //Ptr<MobilityModel> nod6 = nodesCon.Get(0)->GetObject<MobilityModel>();
  //nod6->SetPosition(Vector(boundry.x + 50, boundry.y+35, boundry.z));
 //Ptr<MobilityModel> nod7 = nodesCon.Get(0)->GetObject<MobilityModel>();
  //nod7->SetPosition(Vector(boundry.x + 55, boundry.y+15, boundry.z));
  
  
  Ptr<MobilityModel> sink0 = sinksCon.Get(0)->GetObject<MobilityModel>();
  sink0->SetPosition(Vector(boundry.x + 80, boundry.y+35, boundry.z));


  PacketSocketAddress socket;
  socket.SetAllDevices(); // Set all nodes as sender asides the receiver
  //socket.SetSingleDevice(0); // Set a specific node as the sender which is ---> node 1
  socket.SetPhysicalAddress (devices.Get(nodes)->GetAddress()); //Set dest to first sink
  socket.SetProtocol (0);

  OnOffHelper app ("ns3::PacketSocketFactory", Address (socket));
  app.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  app.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  app.SetAttribute ("DataRate", DataRateValue (m_dataRate));
  app.SetAttribute ("PacketSize", UintegerValue (m_packetSize));


  // Echo source is installed on the source nodes 
  ApplicationContainer apps = app.Install (nodesCon.Get(0));
  apps.Start (Seconds (0.5));
  apps.Stop (Seconds (simStop));


  Ptr<Node> sinkNode = sinksCon.Get(0);
  TypeId psfid = TypeId::LookupByName ("ns3::PacketSocketFactory"); // Socket factory put into use

  Ptr<Socket> sinkSocket = Socket::CreateSocket (sinkNode, psfid);
  sinkSocket->Bind (socket);
  
 
  std::cout << "-----------Running Simulation-----------\n";
  Packet::EnablePrinting (); //for debugging purposes

  Simulator::Stop(Seconds(simStop+1));
  std::ofstream ascii (asciiTraceFile.c_str());
  if (!ascii.is_open()) {
    NS_FATAL_ERROR("Could not open trace file.");
  }
  asHelper.EnableAsciiAll(ascii);
  Simulator::Run();
  
  asHelper.GetChannel()->PrintCounters();
  Simulator::Destroy();

  return 0;
}
