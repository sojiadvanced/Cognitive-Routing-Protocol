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
      .AddAttribute ("HelloInterval", "HELLO messages emission interval.",
                   TimeValue (Seconds (10)),
                   MakeTimeAccessor (&AquaSimCarp::wait_time),
                   MakeTimeChecker ())
    ;
  return tid;
}

/* Node receives Hello packet, updates its hop count and re-broadcast the packet */
void
AquaSimCarp::RecvHello(Ptr<Packet> p)
{
	if(p)
	{
		AquaSimHeader ash;
		Ipv4Header iph;
		p->RemoveHeader(ash);
		p->RemoveHeader(iph);
		AquaSimAddress temp = ash.GetSAddr(); // Neighbor of the receiving node
		
		// The whole idea here is for nodes to keep updating their neighbors anytime a HELLO packet  is received
		//m_ipv4->GetNetDevice().neigh.push_back(temp).m_neighborAddress; 
		//Ptr<NetDevice> dev = m_ipv4->GetNetDevice (
             // m_ipv4->GetInterfaceForAddress (iface.GetLocal ()));
		
		uint8_t numForward = ash.GetNumForwards() + 1;
		ash.SetNumForwards(numForward);
		ash.SetNextHop(AquaSimRouting::GetBroadcast());
		p->AddHeader(iph);
		p->AddHeader(ash);
		Simulator::Schedule(Seconds(0.0),&AquaSimRouting::SendDown,this,p,ash.GetNextHop(),0.0);
	}
}

void 
AquaSimCarp::SendPing (uint32_t m_num_pkt, vector<Neighbor> neigh)
{
  // The CarpHeader would most likely be a struct data type which house its attributes
  AquaSimHeader ash;
  CarpHeader crh;
  crh.pktCounts = m_num_pkt;
  Ptr<Packet> p = Create<Packet>();
  ash.SetSAddr(RaAddr()); // This is used to identify the source node ?
  ash.SetDirection(AquaSimHeader::DOWN);
 	
  p->AddHeader(crh);  
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

void
AquaSimCarp::SetEnergy()
{
	m_energy = 40.0; 	// This is an assumed value of the power rating ( W or kW)
}

void
AquaSimCarp::SetQueue()
{
	m_queue = 4; 	// Every node can accommodate 4 packets in its buffer
}

void
AquaSimCarp::SetLinkQuality
{
	// The logic behind the link quality is set here
}

void
AquaSimCarp::SendPong(Ptr<Packet> p)
{
  NS_LOG_FUNCTION(this);
  AquaSimHeader ash;
  CarpHeader crh;
  Ipv4Header iph; // The CARP header is built on top of the Ipv4Header
  AquaSimPtTag ptag;
  p->RemoveHeader(ash);
  p->RemoveHeader(crh);
  
  AquaSimAddress dest_addr = ash.GetSAddr();

	// Used to set attributes of the PONG packet
	crh.SetPktSrc(RaAddr());  // Set the source of the packet
	crh.SetLinkQuality() // Computes the values of lq to all nodes using the position vector
	crh.SetHopCount(); // Used to determine the hop count of the node from the sink
	crh.SetQueue(); // Indicates the available buffer space at the sender (This could be symmetric across all nodes)
	crh.SetEnergy();
	
	ash.SetSAddr(RaAddr());
	ash.SetDAddr(dest_addr); // This should be sent to the sender of the PING instead of broadcast 
	ash.SetDirection(AquaSimHeader::DOWN);
	ash.SetSize(IP_HDR_LEN + crh.GetPktLen()+size); // Get the application layer size, Dynamic header and IP header
	ash.SetNextHop(dest_addr); // The one hop destination is set as the next hop
	
  
  p->AddHeader(crh);
  p->AddHeader(ash);
  Time jitter = Seconds(m_rand->GetValue()*0.5);
  Simulator::Schedule(jitter,&AquaSimRouting::SendDown,this,p,dest_addr,jitter);
}

/* A node receives a PONG packet, select the neighbor with max lq and forwards the data packet */
void
AquaSimCarp::RecvPong(Ptr<Packet> p)
{
	// Strip each received packet and obtain the lq
	// The node with the maximum lq is selected as the relay node which is used as the DAddr() 
	AquaSimHeader ash;
	CarpHeader crh;
	p->RemoveHeader(ash);
	p->PeekHeader(crh);
	
	Time wait_time; // The wait time must keep decrementing before the sender determines the max lq
	double lq_check = 0.0;
	if (wait_time != 0.0)
	{
	  	double neighbor_lq = crh.GetLinkQuality();
	  	if (lq_check < neighbor_lq)
	  	{
			lq_check = neighbor_lq;
			AquaSimAddress relay_node = ash.GetSAddr();
			ash.GetNextHop(relay_node);
			p->AddHeader(ash);
		}
	  	wait_time--;	
	}
	else
	{
		p->AddHeader(ash);
	}
	ForwardData(p); // This module would be worked upon
	
}

// This is used for receiving actual data packets
bool
AquaSimCarp::Recv(Ptr<Packet> p, const Address &dest, uint16_t protocolNumber)
{
  AquaSimHeader ash;
  Ipv4Header iph;
  CarpHeader crh;
  AquaSimPtTag ptag;
  if (p->GetSize() <= 32)  // What check is this ? 
  {
    p->AddHeader(iph);
    p->AddHeader(crh);
    p->AddHeader(ash);
  }
	p->RemoveHeader(ash);
    p->PeekPacketTag(ptag);
	// This checks if the source address equals the nodeID
	if (ash.GetSAddr() == RaAddr()) {
		// If there exists a loop, must drop the packet, eliminating loop of infinity
		if (ash.GetNumForwards() > 0) {
      NS_LOG_INFO("Recv: there exists a loop, dropping packet=" << p);
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
