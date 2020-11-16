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
	std::vector<AquaSimAddress> m_neighborAddress;
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
  virtual bool Recv(Ptr<Packet> packet);
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
  void SetNextHop(AquaSimAddress src, std::vector<AquaSimAddress> nei);
  void RecvTrain(Ptr<Packet> p);
  void RecvAck(Ptr<Packet> p);
  
  // Sending Data Packet
  Ptr<UniformRandomVariable> m_rand;
  void ForwardData(Ptr<Packet> p);  // This is used to send packets to the mac layer for onward delivery to the destination or next hop
  virtual void DoDispose();

// private:
  Time wait_time;
  Time hello_time = Seconds(1.0);
  AquaSimAddress sAddr;
  uint8_t m_hopCount;
  uint8_t m_num_pkt =4; // An assumption is made for the number of packets
  AquaSimAddress dAddr;
  double m_energy;
  double m_linkQuality;
  uint8_t m_queue;
  double lq; 
  double alpha = 0.85;
  AquaSimAddress m_nextHop;
 // AquaSimNetDevice m_device;
  uint8_t m_nodeId =0;
};  // class AquaSimCarp 
} // End of ns3
