/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Created by KHikami (Rachel Chu) 2017
 **/


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

#include "ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.hpp"

using namespace std;

namespace ns3 {
namespace ndn {


int
main(int argc, char* argv[])
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("10"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(5);

  // Connecting nodes using links between each one as shown in topology map
  PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(1)); //consumer connected to one router
  p2p.Install(nodes.Get(1), nodes.Get(2));
  p2p.Install(nodes.Get(2), nodes.Get(3));//last router connects to "signer"
  p2p.Install(nodes.Get(2), nodes.Get(4));//last router connects to "producer"

  // Install NDN stack on all nodes
  StackHelper ndnHelper;
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Freshness::Lru");
  ndnHelper.InstallAll();

  // Choosing forwarding strategy (can change this later when defining consumer and producer)
  StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/multicast");

  // Install global routing helper on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Installing applications

  std::string dataPrefix = "/prefix/data";
  std::string keyPrefix = "/prefix/key";

  // Consumer
  AppHelper consumerHelper("ns3::ndn::SecurityToyClientApp");
  consumerHelper.SetPrefix(dataPrefix);
  consumerHelper.SetAttribute("WaitTime", StringValue("1.0"));//sends out a new interest for a packet per this int.
  consumerHelper.SetAttribute("ReactionTime", StringValue("0.5"));
  consumerHelper.SetAttribute("KeyName", StringValue(keyPrefix));
  consumerHelper.Install(nodes.Get(0));                        // first node

  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix(dataPrefix);
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(nodes.Get(4));

  ndnGlobalRoutingHelper.AddOrigins(dataPrefix, nodes.Get(4));

  //Signer is a producer that replies with the "key" for Producer's data
  ndn::AppHelper signerHelper("ns3::ndn::Producer");
  signerHelper.SetPrefix(keyPrefix);
  signerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  signerHelper.Install(nodes.Get(3));

  ndnGlobalRoutingHelper.AddOrigins(keyPrefix, nodes.Get(3));

  
  //supposedly initializes and creates fibs
  ndn::GlobalRoutingHelper::CalculateRoutes();

  Simulator::Stop(Seconds(20.0));

  ndn::AppDelayTracer::InstallAll("results/simple-signer-app-delays-trace.txt");

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ndn

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::ndn::main(argc, argv);
}
