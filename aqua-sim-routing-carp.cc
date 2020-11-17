/* Main file for Channel-aware Routing Protocol */

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

using namespace ns3;

/**** AquaSimCarp ****/


/* Carp protocol definition with wait_time constructor value */
AquaSimCarp::AquaSimCarp() : wait_time(MilliSeconds (6.0))
{
 // NS_LOG_FUNCTION(this);
  m_rand = CreateObject<UniformRandomVariable> ();
}

// This is used to create a Type Id during runtime
TypeId
AquaSimCarp::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::AquaSimCarp")
      .SetParent<AquaSimRouting>()
      .AddConstructor<AquaSimCarp>()
      /* Assumptions
       * Speed of sound = 1500m/s
       * Max distance between nodes = 5m
       *  */
      .AddAttribute ("WaitTime", "Time duration to retrieve all the PONG packets. ",
                   TimeValue (MilliSeconds (6.6)),
                   MakeTimeAccessor (&AquaSimCarp::wait_time),
                   MakeTimeChecker ())
      .AddAttribute("HelloTime", "Time duration for the HELLO broadcast. ",
					TimeValue (Seconds (1.0)),
					MakeTimeAccessor (&AquaSimCarp::hello_time),
					MakeTimeChecker ());
  return tid;
}

/* HELLO broadcast by the sink and other sensor nodes in the network topology */
void
AquaSimCarp::SendHello()
{
	//AquaSimNetDevice sink = m_device->GetChannel()->GetNode(m_nodeId); // The need to initiate from the sink
	Ptr<Packet> p = Create<Packet>();
	AquaSimHeader ash;
	HelloHeader hh;
	uint16_t hopCount = ash.GetNumForwards(); // This might need to be stored and mapped with the neighbors data
	hh.SetHopCount(hopCount);  // Assumes the initial hop count of the sink is 0
	// sAddr = AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress());
	sAddr = RaAddr();
	hh.SetSAddr(sAddr);
	
	ash.SetNumForwards(ash.GetNumForwards() + 1);
	ash.SetNextHop(AquaSimAddress::GetBroadcast()); // This is used to broadcast the packet to all neighbors
	p->AddHeader(hh);
	p->AddHeader(ash);
	Simulator::Schedule(Seconds(0.0),&AquaSimRouting::SendDown,this,p,AquaSimAddress::GetBroadcast(),Seconds(0.0));
		
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
		uint16_t numForwards = ash.GetNumForwards();
		
		hh.SetHopCount(numForwards);
		AquaSimAddress temp = hh.GetSAddr(); // Neighbor of the receiving node
		
		/* Two key things are required
		 * 1. The need to obtain the AquaSimNetDevice of the current node with the packet
		 * 2. The node id of the current node
		 *  */
		// Ptr<AquaSimNetDevice> pktNode = m_device->GetChannel()->GetDevice(m_nodeId);
		Address pktNodeAddr = GetNetDevice()->GetAddress();
		m_nodeNeighbor[pktNodeAddr].m_neighborAddress.push_back(temp); // This maps the sender address and updates the neighbors vector holding the neighbor address of the interface

	
		ash.SetNumForwards(numForwards + 1);
		ash.SetNextHop(AquaSimAddress::GetBroadcast());
		p->AddHeader(hh);
		p->AddHeader(ash);
		Simulator::Schedule(Seconds(0.0),&AquaSimRouting::SendDown,this,p,ash.GetNextHop(),Seconds(0.0));
		// SendHello(); Confirm if this method is needed
	}
}

/* Used by communicating nodes to send a PING packet */
void 
AquaSimCarp::SendPing ()
{
  // The CarpHeader would most likely be a struct data type which houses its attributes
  Ptr<Packet> p = Create<Packet>();
  AquaSimHeader ash;
  PingHeader ph; // Header for the PING packet
  ph.SetPktCount(m_numPkt); // Set the number of packets to be sent  
  ph.SetSAddr(RaAddr());
  ash.SetSAddr(RaAddr()); // This is used to identify the source node ?
  ash.SetDirection(AquaSimHeader::DOWN);
  p->AddHeader(ph);  
  
  /* How to obtain the node_Id?
   * The key of the map should be the node Address and not a pointer
   * key,value = Address, Neighbor
   *  */
  Address pktNodeAddr = GetNetDevice()->GetAddress();
  
  // Extract the neighbors of the identified AquaSimNetDevice & iterate through all the neighbors of a node to send the PING packet
  for (std::map<Address, Neighbor>::iterator iter = m_nodeNeighbor.begin();
  iter!= m_nodeNeighbor.end(); iter++)
  {
	if (iter->first == pktNodeAddr)
	{
		for (std::vector<AquaSimAddress>::iterator it = iter->second.m_neighborAddress.begin(); it!= iter->second.m_neighborAddress.end(); it++)
		{  
			ash.SetDAddr(*it);
			ash.SetNextHop(*it);
			p->AddHeader(ash);
			Simulator::Schedule(Seconds(0.0),&AquaSimRouting::SendDown,this,p,ash.GetNextHop(),Seconds(0.0));
	  
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
	AquaSimHeader ash;
	p->RemoveHeader(ash);
	p->AddHeader(ash);
	Simulator::Schedule(Seconds(0.0),&AquaSimRouting::SendDown,this,p,ash.GetNextHop(),Seconds(0.0));
}

void
AquaSimCarp::SendPong(Ptr<Packet> p)
{
  AquaSimHeader ash;
  PingHeader ph; // This is the header used to encapsulate the PING packet
  PongHeader poh;	// Header for PONG packets. This header inherits some of the base CarpHeader methods
  // Ipv4Header iph; // The CARP header is built on top of the Ipv4Header
  
  p->RemoveHeader(ash);
  p->RemoveHeader(ph);
  
  AquaSimAddress dest_addr = ph.GetSAddr();

	// Used to set attributes of the PONG packet
	p->AddHeader(ash);
	// poh.SetHopCount(p); // Used to determine the hop count of the node from the sink
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
	crh.SetPacketType(PckType::ACK);
	crh.SetSAddr(RaAddr());
	ash.SetSAddr(AquaSimAddress::ConvertFrom(m_device->GetAddress())); // This converts the interface address to AquaSimAddress
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
	p->AddHeader(crh);
	AquaSimAddress DataSender = crh.GetSAddr();
	p->AddHeader(crh);
	p->AddHeader(ash);
	SendPacket(MakeACK(DataSender)); // Check this method and reference the ALOHA sent by Dmitrii
	p =0;
}

/* Train of packets received by neighbors for lq computation */
void
AquaSimCarp::RecvTrain(Ptr<Packet> p)
{
	AquaSimHeader ash;
	CarpHeader crh;
	p->RemoveHeader(ash);
	p->RemoveHeader(crh);
	if(crh.GetPacketType() == LQ_DATA)
	{
		p->AddHeader(crh);
		p->AddHeader(ash);
		SendACK(p);
	}
	p =0;
}

/* ACK received by the sender for train of packets to determine the link quality estimate */
void
AquaSimCarp::RecvAck(Ptr<Packet> p)
{
	AquaSimHeader ash;
	CarpHeader crh;
	p->RemoveHeader(ash);
	p->RemoveHeader(crh);
	AquaSimAddress neighbor = ash.GetSAddr();
	if(crh.GetPacketType() == 0)
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
/* Method to select the next hop while awaiting the PONG response */
void
AquaSimCarp::SetNextHop(AquaSimAddress src, std::vector<AquaSimAddress> nei)
{
	
	Ptr<Packet> p = Create<Packet>();
	// vector<double_t>max_lq;
	Time jitter = Seconds(m_rand->GetValue()*0.5);
	uint16_t numForwards = 1;
	
	// Send 4 packets each to all neighbor nodes
	for (std::vector<AquaSimAddress>::iterator it = nei.begin(); it!= nei.end();
	it++)
	{
		pCount.insert(std::pair<AquaSimAddress, int>(*it,0)); // This map automatically keeps track of the ACKS from the neighbors for PSR estimation
		for (uint8_t i = 0; i< 4; i++)
		{
			AquaSimHeader ash;
			// PacketType var = PacketType::LQ_DATA;
			CarpHeader crh;
			crh.SetPacketType(LQ_DATA);
			ash.SetNumForwards(numForwards);
			ash.SetSAddr(src);
			ash.SetDAddr(*it);
			ash.SetNextHop(*it);
			p->AddHeader(crh);
			p->AddHeader(ash);
			Simulator::Schedule(jitter, &AquaSimRouting::SendDown, this, p, ash.GetNextHop(), jitter);	
		}
	}
	Time now = Simulator::Now();
	while(true)
	{
		/* Neighbors receiving a train of packets from sender in order to process an ACK */
		
		RecvTrain(p);
		/* For everytime an Ack is received by a neighbor, the counter is increased */
		RecvAck(p);
		Time new_time = Simulator::Now();
		if ( (new_time - now) > wait_time) // This allows the sender wait for the <wait_time> to elapse before selecting the next relay node
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
	
		// NS_LOG_INFO("The selected relay node has link quality of: " << m_linkQuality);
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
	p->RemoveHeader(poh);
	CarpHeader crh;
	
	// Obtain the source address of the node via the AquaSimNetDevice interface
	// Ptr<AquaSimNetDevice> dev = m_device->GetChannel()->GetDevice(m_nodeId);
	Address pktNodeAddr = GetNetDevice()->GetAddress();
	AquaSimAddress srcAddr = RaAddr();
	Neighbor srcNeighbor;
	srcNeighbor = m_nodeNeighbor[pktNodeAddr]; // Confirm if this process extract the struct Neighbor from the map
	
	SetNextHop(srcAddr, srcNeighbor.m_neighborAddress);
	AquaSimAddress nextHop = GetNextHop(); // This should return the relay node address
	crh.SetPacketType(DATA);
	ash.SetNextHop(nextHop);
	p->AddHeader(crh);
	p->AddHeader(ash);
	  		
	ForwardData(p); // This module would be worked upon	
}
int64_t
AquaSimCarp::AssignStreams (int64_t stream)
{
  // NS_LOG_FUNCTION (this << stream);
  m_rand->SetStream(stream);
  return 1;
}
// This is used for receiving actual data packets
bool
AquaSimCarp::Recv(Ptr<Packet> p, const Address &dest, uint16_t protocolNumber)
{
  AquaSimHeader ash;
  CarpHeader crh;

  p->RemoveHeader(ash);
  p->RemoveHeader(crh);
  
  AquaSimAddress dst = ash.GetDAddr();
	// This checks if the source address equals the nodeID
	if (ash.GetSAddr() == RaAddr()) {
		// If there exists a loop, must drop the packet, eliminating loop of infinity
		if (ash.GetNumForwards() > 0) {
			// NS_LOG_INFO("Recv: there exists a loop, dropping packet =" << p);
			p=0;
			return false;
		}
		// else if this is a packet I am originating, must add IP header
		// else if (ash.GetNumForwards() == 0) // The node is the source of this packet
		//	ash.SetSize(ash.GetSize());
	}
	else if( ash.GetNextHop() != AquaSimAddress::GetBroadcast() && ash.GetNextHop() != RaAddr() )
   {
		// NS_LOG_INFO("Recv: duplicate, dropping packet=" << p);
		p=0;
		return false;
	}
	else if (dst == GetNetDevice()->GetAddress() && crh.GetPacketType() == 1)
	{
		// NS_LOG_INFO("AquaSimCarp::Recv address: " << 
				//	GetNetDevice()->GetAddress() << " packet is delivered ");
		p->AddHeader(ash);
		SendUp(p); // Sends the packet up the application layer
		return true;
	}
  uint16_t numForward = ash.GetNumForwards() + 1;
  ash.SetNumForwards(numForward);
  p->AddHeader(crh);
  p->AddHeader(ash);
  ForwardData(p);
  return true;
}
void 
AquaSimCarp::DoDispose()
{
  m_rand=0;
  AquaSimRouting::DoDispose();
}
