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
 * (consumer) -- ( ) ----- (signer)
 *     |          |         |
 *    ( ) ------ ( ) ----- ( )
 *     |          |         |
 *    ( ) ------ ( ) -- (producer)
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

  // Getting containers for the consumer/producer
  Ptr<Node> producer = grid.GetNode(2, 2);
  NodeContainer consumerNodes;
  consumerNodes.Add(grid.GetNode(0, 0));
  Ptr<Node> signer = grid.GetNode(0,2);

  // Install NDN applications
  std::string dataPrefix = "/prefix/data";
  std::string keyPrefix = "/prefix/key";

  ndn::AppHelper consumerHelper("ns3::ndn::SecurityToyClientApp");
  consumerHelper.SetPrefix(dataPrefix);
  consumerHelper.SetAttribute("WaitTime", StringValue("1.0"));
  consumerHelper.SetAttribute("ReactionTime", StringValue("0.5"));
  consumerHelper.SetAttribute("KeyName", StringValue(keyPrefix));
  consumerHelper.Install(consumerNodes);

  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  producerHelper.SetPrefix(dataPrefix);
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(producer);

  ndn::AppHelper signerHelper("ns3::ndn::Producer");
  signerHelper.SetPrefix(keyPrefix);
  signerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  signerHelper.Install(signer);

  // Add /prefix origins to ndn::GlobalRouter
  ndnGlobalRoutingHelper.AddOrigins(dataPrefix, producer);
  ndnGlobalRoutingHelper.AddOrigins(keyPrefix, signer);

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();

  Simulator::Stop(Seconds(20.0));

  ndn::AppDelayTracer::InstallAll("results/grid-signer-app-delays-trace.txt");

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
