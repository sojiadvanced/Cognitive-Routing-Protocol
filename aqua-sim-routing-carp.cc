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
	//CarpHeader crh;
	HelloHeader hh; // This is an object of the HelloHeader
	//p->RemoveHeader(ash);
	//p->RemoveHeader(crh);
	hh.SetHopCount(hopCount);
	hh.SetSAddr(sAddr);
	
	p->AddHeader(hh);
	ash.SetNextHop(AquaSimRouting::GetBroadcast()); // This is used to broadcast the packet to all neighbors
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
		HelloHeader hh;
		p->RemoveHeader(ash);
		p->RemoveHeader(hh);
		AquaSimAddress temp = hh.GetSAddr(); // Neighbor of the receiving node
		
		// The whole idea here is for nodes to keep updating their neighbors anytime a HELLO packet  is received
		m_if  = m_ipv4->GetInterfaceForAddress (iface.GetLocal ())); // This is used to retrive the interface id for the local address
		Ptr<NetDevice> dev = m_ipv4->GetNetDevice(m_if); // The netdevice for the local address interface is obtained 
        
        /* neighbor cache that is associated with a NetDevice)
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
  // The CarpHeader would most likely be a struct data type which house its attributes
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
	p->RemoveHeader(ash);
	Simulator::Schedule(Seconds(0.0),&AquaSimRouting::SendDown,this,p,ash.GetNextHop(),0.0);
}
///* The energy levels of the nodes are set using this module */
//void
//AquaSimCarp::SetEnergy()
//{
	//m_energy = 40.0; 	// This is an assumed value of the power rating ( W or kW)
//}

///* This is used to set the maximum value of the receiver's buffer for congestion control */
//void
//AquaSimCarp::SetQueue()
//{
	//m_queue = 4; 	// Every node can accommodate 4 packets in its buffer
//}

//void
//AquaSimCarp::SetLinkQuality
//{
	//// The logic behind the link quality is set here
//}

/* This is used to send PONG packets by neighbors to its sender */
void
AquaSimCarp::SendPong(Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this);
  AquaSimHeader ash;
  PingHeader ph; // This is the header used to encapsulate the PING packet
  PongHeader poh;	// Header for PONG packets. This header inherits some of the base CarpHeader methods
  Ipv4Header iph; // The CARP header is built on top of the Ipv4Header
  
  p->RemoveHeader(ash);
  p->RemoveHeader(ph);
  
  AquaSimAddress dest_addr = ph.GetSAddr();

	// Used to set attributes of the PONG packet
	poh.SetSAddr(RaAddr());  // Set the source of the packet
	poh.SetLinkQuality() // Computes the values of lq to all nodes using the position vector
	poh.SetHopCount(); // Used to determine the hop count of the node from the sink
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
	
	Time wait_time; // The wait time must keep decrementing before the sender determines the max lq
	double lq_check = 0.0;
	while (wait_time != 0.0)
	{
	  	double neighbor_lq = poh.GetLinkQuality();
	  	if (lq_check < neighbor_lq)
	  	{
			lq_check = neighbor_lq;
			AquaSimAddress relay_node = ash.GetSAddr();
			ash.GetNextHop(relay_node);
			p->AddHeader(ash);
		}
	  	wait_time--;	
	}
	ForwardData(p); // This module would be worked upon	
}

// This is used for receiving actual data packets
bool
AquaSimCarp::Recv(Ptr<Packet> p)
{
  AquaSimHeader ash;
  Ipv4Header iph;
  CarpHeader crh;
//  if (p->GetSize() <= 32)  // What check is this ? 
 // {
  //  p->AddHeader(iph);
  //  p->AddHeader(crh);
  //  p->AddHeader(ash);
 // }
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
