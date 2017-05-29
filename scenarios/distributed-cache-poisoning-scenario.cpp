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
#include "ns3/point-to-point-layout-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/utils/tracers/ndn-app-delay-tracer.hpp"

namespace ns3 {

/**
 * This scenario simulates a grid topology (using PointToPointGrid module)
 *
 * (consumer1) -- ( ) ----(signer)
 *      |          |         |
 * (consumer2) -- ( ) ---  (evil)
 *      |          |         |
 * (consumer3) -- ( ) -- (producer)
 *
 * All links are 1Mbps with propagation 10ms delay.
*/

int
main(int argc, char* argv[])
{
  // Setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("10"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating 3x3 topology
  PointToPointHelper p2p;
  PointToPointGridHelper grid(3, 3, p2p);
  grid.BoundingBox(100, 100, 200, 200);

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Freshness::Lru");
  ndnHelper.InstallAll();

  // Set BestRoute strategy
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");

  // Installing global routing interface on all nodes
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Labeling which node is which.
  Ptr<Node> goodProducer = grid.GetNode(2,2);
  Ptr<Node> evilProducer = grid.GetNode(1,2);
  Ptr<Node> signer = grid.GetNode(0,2);
  Ptr<Node> consumer1= grid.GetNode(0,0);
  Ptr<Node> consumer2= grid.GetNode(1,0);
  Ptr<Node> consumer3= grid.GetNode(2,0);

  // Install NDN applications
  std::string dataPrefix = "/prefix/data";
  std::string keyPrefix = "/prefix/key";
  std::string goodPayloadSize = "1024";

  ndn::AppHelper consumerHelper("ns3::ndn::SecurityToyClientApp");
  consumerHelper.SetPrefix(dataPrefix);
  consumerHelper.SetAttribute("WaitTime", StringValue("2"));
  consumerHelper.SetAttribute("KeyName", StringValue(keyPrefix));
  consumerHelper.Install(consumer1);

  //Consumer2: delay start time of 1
  ndn::AppHelper consumerHelper2("ns3::ndn::SecurityToyClientApp");
  consumerHelper2.SetPrefix(dataPrefix);
  consumerHelper2.SetAttribute("WaitTime", StringValue("2"));
  consumerHelper2.SetAttribute("KeyName", StringValue(keyPrefix));
  consumerHelper2.SetAttribute("DelayStart", StringValue("1"));
  consumerHelper2.Install(consumer2);

  //Consumer3: delay start time of 2
  ndn::AppHelper consumerHelper3("ns3::ndn::SecurityToyClientApp");
  consumerHelper3.SetPrefix(dataPrefix);
  consumerHelper3.SetAttribute("WaitTime", StringValue("2"));
  consumerHelper3.SetAttribute("KeyName", StringValue(keyPrefix));
  consumerHelper3.SetAttribute("DelayStart", StringValue("2"));
  consumerHelper3.Install(consumer3);

  //Good Producer
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix(dataPrefix);
  producerHelper.SetAttribute("PayloadSize", StringValue(goodPayloadSize));
  producerHelper.Install(goodProducer);

  ndnGlobalRoutingHelper.AddOrigins(dataPrefix, goodProducer);

  //Evil Producer
  ndn::AppHelper evilHelper("ns3::ndn::EvilProducerApp");
  evilHelper.SetPrefix(dataPrefix);
  evilHelper.SetAttribute("PayloadSize", StringValue("1000"));
  evilHelper.Install(evilProducer);
  ndnGlobalRoutingHelper.AddOrigins(dataPrefix, evilProducer);

  //Signer
  ndn::AppHelper signerHelper("ns3::ndn::Producer");
  signerHelper.SetPrefix(keyPrefix);
  signerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  signerHelper.Install(signer);

  ndnGlobalRoutingHelper.AddOrigins(keyPrefix, signer);

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();

  Simulator::Stop(Seconds(20.0));

  ndn::AppDelayTracer::InstallAll("results/distributed-cache-poisoning-app-delays-trace.txt");

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
