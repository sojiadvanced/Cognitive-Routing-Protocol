/* Main file for channel-aware routing protocol */

#include "aqua-sim-routing-carp.h"
#include "aqua-sim-header-routing.h"
#include "aqua-sim-header.h"
#include "aqua-sim-pt-tag.h"
#include "aqua-sim-propagation.h"
#include "ns3/log.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "ns3/mobility-model.h"
#include "ns3/simulator.h"

#define IP_HDR_LEN 20
using namespace ns3;

/**** AquaSimCarp ****/

NS_OBJECT_ENSURE_REGISTERED(AquaSimCarp);

/* Carp protocol definition with wait_time constructor value */
AquaSimCarp::AquaSimCarp() : wait_time(this, 10) // Check why the need of m_pktTimer
{
  NS_LOG_FUNCTION(this);
  m_rand = CreateObject<UniformRandomVariable> ();
}

// This is used to create a Type Id during runtime
TypeId
AquaSimCarp::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimCarp")
      .SetParent<AquaSimRouting>()
      .AddConstructor<AquaSimCarp>()
      .AddAttribute ("WaitTime", "Time duration to retrieve all the PONG packets. ",
                   TimeValue (Seconds (10)),
                   MakeTimeAccessor (&AquaSimCarp::wait_time),
                   MakeTimeChecker ())
    ;
  return tid;
}

/* HELLO broadcast by the sink and other sensor nodes in the network topology */
void
AquaSimCarp::SendHello()
{
	Ptr<Packet> p = CreatePacket();
	AquaSimHeader ash;
	Ipv4Header iph;
	HelloHeader hh; // This is an object of the HelloHeader
	
	hh.SetHopCount(hopCount);  // Assumes the initial hop count of the sink is 0
	// uint8_t nodeId
	// How to get the node_id ?
	AquaSimNetDevice sink = m_device->GetChannel()->GetNode(m_nodeId);
	sAddr = AquaSimAddress::ConvertFrom(m_device->GetAddress());
	hh.SetSAddr(sAddr);
	
	p->AddHeader(hh);
	ash.SetNextHop(AquaSimRouting::GetBroadcast()); // This is used to broadcast the packet to all neighbors
	p->Addheader(iph);
	p->AddHeader(ash);
	Simulator::Schedule(Seconds(0.0),&AquaSimRouting::SendDown,this,p,ash.GetNextHop(),0.0);	
}

/* Node receives Hello packet, updates its hop count and re-broadcast the packet */
void
AquaSimCarp::RecvHello(Ptr<Packet> p)
{
	if(p)
	{
		AquaSimHeader ash;
		Ipv4Header iph;
		HelloHeader hh;
		p->RemoveHeader(ash);
		p->RemoveHeader(iph); // This is used to access the Ipv4 object so as to obtain the local interface
		p->RemoveHeader(hh);
		AquaSimAddress temp = hh.GetSAddr(); // Neighbor of the receiving node
		
		// The whole idea here is for nodes to keep updating their neighbors anytime a HELLO packet  is received
		// How to parse the m_nodeId?
		Ptr<AquaSimNetDevice> dev = m_device->GetChannel()->GetDevice(m_nodeId);
        
        /* neighbor cache that is associated with a NetDevice
         * src: https://www.nsnam.org/doxygen/classns3_1_1_ipv4.html#details
         * */
        m_nodeNeighbor[dev] = m_neigh->m_neighborAddress.push_back(temp); // This maps the sender address and updates the vector holding the neighbor address of the interface
		uint8_t numForward = ash.GetNumForwards() + 1;
		ash.SetNumForwards(numForward);
		ash.SetNextHop(AquaSimRouting::GetBroadcast());
		p->AddHeader(hh);
		p->AddHeader(ash);
		Simulator::Schedule(Seconds(0.0),&AquaSimRouting::SendDown,this,p,ash.GetNextHop(),0.0);
	}
}

/* Used by communicating nodes to send a PING packet */
void 
AquaSimCarp::SendPing (uint32_t m_num_pkt, vector<Neighbor> neigh)
{
  // The CarpHeader would most likely be a struct data type which houses its attributes
  AquaSimHeader ash;
  PingHeader ph; // Header for the PING packet
  ph.SetPktCount(m_num_pkt); // Set the number of packets to be sent 
  Ptr<Packet> p = Create<Packet>();
  ph.SetSAddr(RaAddr());
  ash.SetSAddr(RaAddr()); // This is used to identify the source node ?
  ash.SetDirection(AquaSimHeader::DOWN);
 	
  p->AddHeader(ph);  
  // This iterates through all the neighbors of a node and send the PING packet
  for (vector<Neighbor> iterator it = neigh.begin(); it!= neigh.end(); it++)
  {
	  
	ash.SetDAddr(it->m_neighborAddress);
	ash.SetNextHop(it->m_neighborAddress);
	p->AddHeader(ash);
	Simulator::Schedule(Seconds(0.0),&AquaSimRouting::SendDown,this,p,ash.GetNextHop(),0.0);
	  
  }
}

/* Every node that receives a PING packet, calls the SendPong() module */
void 
AquaSimCarp::RecvPing (Ptr<Packet> p)
{
	SendPong(p);
}
/* Forward Data Packet */
void 
AquaSimCarp::ForwardData(Ptr<Packet> p)
{
	AquaSimAddress ash;
	p->PeekHeader(ash);
	Simulator::Schedule(Seconds(0.0),&AquaSimRouting::SendDown,this,p,ash.GetNextHop(),0.0);
}

void
AquaSimCarp::SendPong(Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this);
  AquaSimHeader ash;
  PingHeader ph; // This is the header used to encapsulate the PING packet
  PongHeader poh;	// Header for PONG packets. This header inherits some of the base CarpHeader methods
  // Ipv4Header iph; // The CARP header is built on top of the Ipv4Header
  
  p->RemoveHeader(ash);
  p->RemoveHeader(ph);
  
  AquaSimAddress dest_addr = ph.GetSAddr();

	// Used to set attributes of the PONG packet
	p->AddHeader(ash);
	poh.SetHopCount(p); // Used to determine the hop count of the node from the sink
	p->RemoveHeader(ash);
	poh.SetSAddr(RaAddr());  // Set the source of the packet
	
	// This process might be skipped because of the complexity
	// poh.SetLinkQuality(Ptr<Neighbor> neig) // Computes the values of lq to all nodes using the position vector (Args: NetDevice, Nodes, Neighbors)
	
	poh.SetQueue(m_queue); // Indicates the available buffer space at the sender (This could be symmetric across all nodes)
	poh.SetEnergy(m_energy);
	poh.SetDAddr(dest_addr);
	
	ash.SetSAddr(RaAddr());
	ash.SetDAddr(dest_addr); // This should be sent to the sender of the PING instead of broadcast 
	ash.SetDirection(AquaSimHeader::DOWN);
	ash.SetNextHop(dest_addr); // The one hop destination is set as the next hop
  
  p->AddHeader(poh);
  p->AddHeader(ash);
  Time jitter = Seconds(m_rand->GetValue()*0.5);
  Simulator::Schedule(jitter,&AquaSimRouting::SendDown,this,p,dest_addr,jitter);
}

/* This method is used to create an ACK */
Ptr<Packet>
AquaSimCarp::MakeACK(AquaSimAddress DataSender)
{
	Ptr<Packet> p = Create<Packet>();
	AquaSimHeader ash;
	CarpHeader crh;
	crh.SetPacketType(ACK);
	ash.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress()); // This converts the interface address to AquaSimAddress
	ash.SetNextHop(DataSender);
	ash.SetDAddr(DataSender);
	p->AddHeader(crh);
	p->AddHeader(ash);
	
	return p;
}
/* This method is used to reply the sender with an ACK upon receiving a packet */
void
AquaSimCarp::SendACK(Ptr<Packet> p)
{
	CarpHeader crh;
	AquaSimHeader ash;
	p->RemoveHeader(ash);
	p->PeekHeader(crh);
	AquaSimAddress DataSender = crh.GetSAddr();
	p->AddHeader(ash);
	SendPacket(MakeACK(DataSender)); // Check this method and reference the ALOHA sent by Dmitrii
	// m_boCounter = 0;
	// p =0;
}
/* A node receives a PONG packet, selects the neighbor with max lq and forwards the data packet */
void
AquaSimCarp::RecvPong(Ptr<Packet> p)
{
	// Strip each received packet and obtain the lq
	// The node with the maximum lq is selected as the relay node which is used as the DAddr() 
	AquaSimHeader ash;
	PongHeader poh;
	p->RemoveHeader(ash);
	p->PeekHeader(poh);
	CarpHeader crh;
	
	Time wait_time; // The wait time must keep decrementing before the sender determines the max lq
	double lq_check = 0.0;
	
	// Obtain the source address of the node via the AquaSimNetDevice interface
	Ptr<AquaSimNetDevice> dev = m_device->GetChannel()->GetDevice(m_nodeId);
	AquaSimAddress srcAddr = RaAddr();
	Neighbor srcNeighbor;
	srcNeighbor = m_nodeNeighbor[dev];
	while (wait_time != 0.0)
	{
		// This is from each neighbor of x to the best reachable neighbor towards the sink
	  	//double neighbor_lq = poh.GetLinkQuality();
	  	//if (lq_check < neighbor_lq)
	  	//{
			//lq_check = neighbor_lq;
			//AquaSimAddress relay_node = ash.GetSAddr();
			//ash.GetNextHop(relay_node);
			//p->AddHeader(ash);
		//}
		// To compute lq_x,y (This is the link quality estimate from x to y)
		crh.SetLinkQuality(srcAddr, srcNeighbor.m_neighborAddress);
		AquaSimAddress nextHop = crh.GetLinkQuality(); // This should return the relay node address
		ash.SetNextHop(nextHop);
		p->AddHeader(crh);
		p->AddHeader(ash);
	  	wait_time--;	
	}
	ForwardData(p); // This module would be worked upon	
}

// This is used for receiving actual data packets
bool
AquaSimCarp::Recv(Ptr<Packet> p)
{
  AquaSimHeader ash;
  // Ipv4Header iph; // Not sure if the Ipv4Header is needed
  CarpHeader crh;

	p->RemoveHeader(ash);
	// This checks if the source address equals the nodeID
	if (ash.GetSAddr() == RaAddr()) {
		// If there exists a loop, must drop the packet, eliminating loop of infinity
		if (ash.GetNumForwards() > 0) {
      NS_LOG_INFO("Recv: there exists a loop, dropping packet =" << p);
      p=0;
			return false;
		}
		// else if this is a packet I am originating, must add IP header
		else if (ash.GetNumForwards() == 0) // The node is the source of this packet
			ash.SetSize(ash.GetSize() + IP_HDR_LEN);
	}
	else if( ash.GetNextHop() != AquaSimAddress::GetBroadcast() && ash.GetNextHop() != RaAddr() )
  {
    NS_LOG_INFO("Recv: duplicate, dropping packet=" << p);
    p=0;
		return false;
	}
  uint8_t numForward = ash.GetNumForwards() + 1;
  ash.SetNumForwards(numForward);
  p->AddHeader(ash);
  
  return true;
}


// How methods are called that compute the fields in the packet headers for PONG

//m_pktTimer.Schedule(Seconds(0.0000001+10*m_rand->GetValue()));
