/* Development of Channel-aware Routing Protocol using Aqua-Sim */

//#ifndef AQUA_SIM_ROUTING_CARP_H
//#define AQUA_SIM_ROUTING_CARP_H

#include "aqua-sim-routing.h"
#include "aqua-sim-header-routing.h"
#include "aqua-sim-address.h"
#include "aqua-sim-datastructure.h"
#include "aqua-sim-channel.h"
#include "ns3/vector.h"
#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"
#include <map>
#include <bits/stdc++.h>
#include <vector>

namespace ns3{

// class CarpHeader;

struct Neighbor
{
	//std::vector<AquaSimAddress> m_neighborAddress;
	std::map<AquaSimAddress, uint16_t> m_neighbor; // A container to store the address and hop count of neighbors from the sink
	
};

//Neighbor() : m_neighborAddress(0)
//{
//}

class AquaSimCarp : public AquaSimRouting {
public:
  AquaSimCarp();
  std::map<Address, Neighbor>m_nodeNeighbor; // This is used to retrieve the neighbors of a node via the netdevice
  Neighbor* m_neigh; // This is a pointer to the struct holding the neighbors of each node
  static TypeId GetTypeId(void);
  bool Recv(Ptr<Packet> packet, const Address &dest, uint16_t protocolNumber);
  int64_t AssignStreams (int64_t stream);
  inline AquaSimAddress RaAddr() { return AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()); }
  std::map<AquaSimAddress, int>pCount;
  
  // Processing of Ping Packet
  void SendPing ();
  void RecvPing (Ptr<Packet> packet);

  // Processing of Hello Packet
  void SendHello ();
  void RecvHello (Ptr<Packet> packet);

  // Processing of Pong Packet
  void SendPong (Ptr<Packet> packet);
  void RecvPong (Ptr<Packet> packet); // Neighbors for each node are determined here
  
  // Auxiliary methods
  Ptr<Packet> MakeACK(AquaSimAddress src);
  void SendACK(Ptr<Packet> p);
  AquaSimAddress GetNextHop();
  void SetNextHop(AquaSimAddress src, std::map<AquaSimAddress, uint16_t> nei);
  void RecvTrain(Ptr<Packet> p);
  void RecvAck(Ptr<Packet> p);
  
  // Sending Data Packet
  Ptr<UniformRandomVariable> m_rand;
  void ForwardData(Ptr<Packet> p);  // This is used to send packets to the mac layer for onward delivery to the destination or next hop
  void DoDispose();

// private:
  Time wait_time;
  Time hello_time = Seconds(1.0);
  AquaSimAddress sAddr;
  uint8_t m_hopCount;
  uint8_t m_numPkt =4; // An assumption is made for the number of packets
  AquaSimAddress dAddr;
  double m_energy;
  double m_linkQuality;
  uint8_t m_queue;
  double lq; 
  double alpha = 0.85;
  AquaSimAddress m_nextHop;
  uint8_t m_nodeId =0;
};  // class AquaSimCarp 
} // End of ns3
