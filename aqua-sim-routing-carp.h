/* Development of Channel-aware Routing Protocol using Aqua-Sim */

#ifndef AQUA_SIM_ROUTING_CARP_H
#define AQUA_SIM_ROUTING_CARP_H

#include "aqua-sim-routing.h"
#include "aqua-sim-address.h"
#include "aqua-sim-datastructure.h"
#include "aqua-sim-channel.h"
#include "ns3/vector.h"
#include "ns3/random-variable-stream.h"
#include "ns3/packet.h"
#include <map>

namespace ns3{

// class CarpHeader;

struct Neighbor
{
	
	vector<AquaSimAddress> m_neighborAddress;
	Neighbor : m_neighborAddress(0) {}

};


class AquaSimCarp : public AquaSimRouting {
public:
  AquaSimCarp();
  std::map<Ptr<NetDevice>, Neighbor>m_nodeNeighbor; // This is used to retrieve the neighbors of a node via the netdevice
  Neighbor* m_neigh; // This is a pointer to the struct holding the neighbors of each node
  static TypeId GetTypeId(void);
  virtual bool Recv(Ptr<Packet> packet);
  inline AquaSimAddress RaAddr() { return AquaSimAddress::ConvertFrom(GetNetDevice()->GetAddress()); }
  
  // Processing of Ping Packet
  void SendPing (uint32_t m_num_pkt, vector<Neighbor> neigh);
  void RecvPing (Ptr<Packet> packet);

  // Processing of Hello Packet
  void SendHello ();
  void RecvHello (Ptr<Packet> packet);

  // Processing of Pong Packet
  void SendPong (Ptr<Packet> packet);
  void RecvPong (Ptr<Packet> packet); // Neighbors for each node are determined here
  //double Calculatelq (double lq);	// Calculate the link quality
  
  // Sending Data Packet
  Ptr<UniformRandomVariable> m_rand;

  virtual void DoDispose();

private:
  Ptr<Ipv4>m_ipv4;
  uint32_t m_if; // Used to retrieve the interface from the address
  Time wait_time;
  AquaSimAddress sAddr;
  uint32_t m_hopCount;
  uint32_t m_num_pkt =4; // An assumption is made for the number of packets
  AquaSimAddress dAddr;
  double m_energy;
  double m_linkQuality;
  uint32_t m_queue;
  double lq; 
  double alpha = 0.85;
};  // class AquaSimCarp 
} // End of ns3
