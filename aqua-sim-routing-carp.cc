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

AquaSimCarp::AquaSimCarp() : m_pktTimer(this, 50) // Check why the need of m_pktTimer
{
  NS_LOG_FUNCTION(this);
  m_coun=0;

  m_rTable.SetRouting(this);

  m_pktTimer.SetFunction(&AquaSimCarp_PktTimer::Expire,&m_pktTimer);
  m_pktTimer.Schedule(Seconds(0.0000001+10*m_rand->GetValue()));
  m_rand = CreateObject<UniformRandomVariable> ();
}

// This is used to create a Type Id during runtime
TypeId
AquaSimCarp::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimCarp")
      .SetParent<AquaSimRouting>()
      .AddConstructor<AquaSimCarp>()
      .AddAttribute("AccessibleVar", "Accessible Variable.",
        IntegerValue(0),
        MakeIntegerAccessor (&AquaSimCarp::m_accessibleVar),
        MakeIntegerChecker<int> ())
    ;
  return tid;
}

void
AquaSimCarp::RecvHello(Ptr<Packet> p)
{
	if(p)
	{
		AquaSimHeader ash;
		p->PeekHeader(ash);
		AquaSimAddress temp = ash.GetSAddr(); // Neighbor of the receiving node
		m_netDevice.neighbor.push_back(temp); // The whole idea here is for nodes to keep updating their neighbors anytime a HELLO packet 
		// is received
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
  ash.SetSAddr(RaAddr()); // What is the function of the RaAddr() module ?
  ash.SetDirection(AquaSimHeader::DOWN);
  ash.SetNextHop(AquaSimAddress::GetBroadcast());
	
  p->AddHeader(crh);
  p->AddHeader(ash);
  Simulator::Schedule(Seconds(0.0),&AquaSimRouting::SendDown,this,p,ash.GetNextHop(),0.0);
}

void
AquaSimCarp::SetEnergy()
{
	m_energy = 40.0; 	// This is an assumed value of the power rating
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
  Ipv4Header iph;
  AquaSimPtTag ptag;
  p->RemoveHeader(ash);
  p->RemoveHeader(crh);
  
  AquaSimAddress dest_addr = ash.GetDAddr();

	// Used to set attributes of the PONG packet
	crh.SetPktSrc(RaAddr());  // Set the source of the packet
	crh.SetLinkQuality() // Computes the values of lq to all nodes using the position vector
	crh.SetHopCount(); // Used to determine the hop count of the node from the sink
	crh.SetQueue(); // Indicates the available buffer space at the sender (This could be symmetric across all nodes)
	crh.SetEnergy();
	
	ash.SetSAddr(RaAddr());
	ash.SetDAddr(AquaSimAddress::GetBroadcast()); // This should be sent to the sender of the PING instead of broadcast 
	ash.SetDirection(AquaSimHeader::DOWN);
	ash.SetSize(IP_HDR_LEN + crh.GetPktLen()+size); // Get the application layer size, Dynamic header and IP header
	ash.SetNextHop(AquaSimAddress::GetBroadcast()); // It is assumed that all neighbors are reachable via broadcast
	
  // p->AddHeader(iph);
  p->AddHeader(crh);
  p->AddHeader(ash);
  Time jitter = Seconds(m_rand->GetValue()*0.5);
  Simulator::Schedule(jitter,&AquaSimRouting::SendDown,this,p,dest_addr,jitter);
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
