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
AquaSimCarp::AquaSimCarp() : wait_time(this, 100) // Check why the need of m_pktTimer
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
                   TimeValue (Seconds (100)),
                   MakeTimeAccessor (&AquaSimCarp::wait_time),
                   MakeTimeChecker ());
  return tid;
}

/* HELLO broadcast by the sink and other sensor nodes in the network topology */
void
AquaSimCarp::SendHello()
{
	//AquaSimNetDevice sink = m_device->GetChannel()->GetNode(m_nodeId); // The need to initiate from the sink
	Ptr<Packet> p = CreatePacket();
	AquaSimHeader ash;
	HelloHeader hh;
	uint16_t hopcount = ash.GetNumForwards(); // This might need to be stored and mapped with the neighbors data
	hh.SetHopCount(hopCount);  // Assumes the initial hop count of the sink is 0
	sAddr = AquaSimAddress::ConvertFrom(m_device->GetAddress());
	hh.SetSAddr(sAddr);
	
	p->AddHeader(hh);
	ash.SetNumForwards(ash.GetNumForwards() + 1);
	ash.SetNextHop(AquaSimAddress::GetBroadcast()); // This is used to broadcast the packet to all neighbors
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
		p->PeekHeader(hh);
		uint16_t numForwards = ash.GetNumForwards();
		
		hh.SetHopCount(numForwards);
		AquaSimAddress temp = hh.GetSAddr(); // Neighbor of the receiving node
		// How to parse the m_nodeId?
		Ptr<AquaSimNetDevice> dev = m_device->GetChannel()->GetDevice(m_nodeId);

        m_nodeNeighbor[dev] = m_neigh->m_neighborAddress.push_back(temp); // This maps the sender address and updates the vector holding the neighbor address of the interface
		ash.SetNumForwards(numForwards + 1);
		ash.SetNextHop(AquaSimRouting::GetBroadcast());
		p->AddHeader(ash);
		Simulator::Schedule(Seconds(0.0),&AquaSimRouting::SendDown,this,p,ash.GetNextHop(),0.0);
		// SendHello(); Confirm if the need of this method 
	}
}

/* Used by communicating nodes to send a PING packet */
void 
AquaSimCarp::SendPing ()
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
  
  // How to obtain the node_Id?
  Ptr<AquaSimNetDevice> dev = m_device->GetChannel()->GetDevice(m_nodeId); // Obtain the AquaSimNetDevice of the node
  
  // Extract the neighbors of the identified AquaSimNetDevice & iterate through all the neighbors of a node to send the PING packet
  for (map<Ptr<AquaSimNetDevice>, Neighbor>::iterator iter = m_nodeNeighbor.begin();
  iter!= m_nodeNeighbor.end(); iter++)
  {
	if (iter->first == dev)
	{
		for (vector<AquaSimAddress>::iterator it = iter->second.begin(); it!= iter->second.end(); it++)
		{  
			ash.SetDAddr(*it);
			ash.SetNextHop(*it);
			p->AddHeader(ash);
			Simulator::Schedule(Seconds(0.0),&AquaSimRouting::SendDown,this,p,ash.GetNextHop(),0.0);
	  
		}
	}
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
	// ash.SetDirection(AquaSimHeader::DOWN);
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
	crh.SetSAddr();
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
	p =0;
}
/* Method to select the next hop while awaiting the PONG response */
void
AquaSimCarp::SetNextHop(AquaSimAddress src, vector<AquaSimAddress> nei)
{
	
	vector<double_t>max_lq;
	Time jitter = Seconds(m_rand->GetValue()*0.5);
	uint8_t numForwards = 1;
	
	// Send 4 packets each to all neighbor nodes
	for (vector<AquaSimAddress>::iterator it = nei.begin(); it!= nei.end();
	it++)
	{
		pCount.insert(std::pair<AquaSimAddress, int>(*it,0)); // This map automatically keeps track of the ACKS from the neighbors for PSR estimation
		for (uint8_t i = 0; i< 4; i++)
		{
			AquaSimAddress ash;
			PacketType var = LQ_DATA;
			CarpHeader crh;
			crh.SetPacketType(var);
			Ptr<Packet> p = Create<Packet>();
			ash.SetNumForwards(numForwards);
			ash.SetSAddr(src);
			ash.SetDAddr(*it);
			ash.SetNextHop(*it);
			p->AddHeader(crh);
			p->AddHeader(ash);
			Simulator::Schedule(jitter, &AquaSimRouting::SendDown, this, p, ash.GetNextHop(), jitter);	
			// Find a way for a random delay
			Simulator::Schedule(jitter);
		}
	}
	Time now = Simulator::Now();
	while(true)
	{
		/* Neighbors receiving a train of packets from sender in order to process an ACK */
		void
		AquaSimCarp::RecvTrain(Ptr<Packet> p)
		{
			AquaSimAddress ash;
			CarpHeader crh;
			p->RemoveHeader(ash);
			p->PeekHeader(crh)
			if(p.GetPacketType == LQ_DATA)
			{
				p->AddHeader(ash);
				SendAck(p);
			}
			p =0;
		}
		/* For everytime an Ack is received by a neighbor, the counter is increased */
		void
		AquaSimCarp::RecvAck(Ptr<Packet> p)
		{
			AquaSimAddress ash;
			CarpHeader crh;
			p->RemoveHeader(ash);
			p->PeekHeader(crh);
			AquaSimAddress neighbor = ash.GetSAddr();
			if(crh.GetPacketType == ACK)
			{
				for (std::map<AquaSimAddress, int>::iterator it = pCount.begin(); it!= pCount.end(); it++)
				{
					if(it->first == neighbor)
					{
						it->second = it->second + 1;
					}
				}
			}
		}
		Time new_time = Simulator::Now();
		if ( (new_time - now) > wait_time) // This allows the sender wait for 100 seconds before selecting the next relay node
		{
			break;
		}
	}
		int testVal = 0;
		for (std::map<AquaSimAddress, int>::iterator it = pCount.begin(); it!= pCount.end(); it++)
		{
			if(it->second > testVal)
			{
				testVal = it->second;
			}
		}
		std::map<AquaSimAddress, int>::iterator valnextHop = pCount.find(testVal);
		double_t psr = testVal /4;
		m_linkQuality = psr *alpha;
		m_nextHop = valnextHop->first;  // This is the selected relay node with maximum lq at time <t>
	
		NS_LOG_INFO("The selected relay node has link quality of: " << m_linkQuality);
}
/* This method is to obtain the relay node */
AquaSimAddress
AquaSimCarp::GetNextHop()
{
	return m_nextHop;
}
/* A node receives a PONG packet, selects the neighbor with max lq and forwards the data packet */
void
AquaSimCarp::RecvPong(Ptr<Packet> p)
{
	// The node with the maximum lq is selected as the relay node which is used as the DAddr() 
	AquaSimHeader ash;
	PongHeader poh;
	p->RemoveHeader(ash);
	p->PeekHeader(poh);
	CarpHeader crh;
	
	// Obtain the source address of the node via the AquaSimNetDevice interface
	Ptr<AquaSimNetDevice> dev = m_device->GetChannel()->GetDevice(m_nodeId);
	AquaSimAddress srcAddr = RaAddr();
	Neighbor srcNeighbor;
	srcNeighbor = m_nodeNeighbor[dev]; // Confirm if this process extract the struct Neighbor from the map
	
	SetNextHop(srcAddr, srcNeighbor.m_neighborAddress);
	AquaSimAddress nextHop = GetNextHop(); // This should return the relay node address
	ash.SetNextHop(nextHop);
	p->AddHeader(crh);
	p->AddHeader(ash);
	  		
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

//m_pktTimer.Schedule(Seconds(0.0000001+10*m_rand->GetValue()));
