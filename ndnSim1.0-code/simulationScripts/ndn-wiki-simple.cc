/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */ /*
 * Copyright (c) 2016 University of Luxembourg 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Salvatore Signorello  <salvatore.signorello@uni.lu>
 * Some code neither licensed nor public was provided by:
 */

// Simple simulation script to test the wiki-pagetitle class and the consumer/producer apps making use of it

#include "ns3/wiki-pagetitle.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

using namespace ns3;

int
main(int argc, char* argv[])
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("20"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
    CommandLine cmd;
    cmd.Parse(argc, argv);

    WikiPageTitles::Instance()->load(argv[1]);
    // Creating nodes
    NodeContainer nodes;
    nodes.Create(3);

    // Connecting nodes using two links
    PointToPointHelper p2p;
    p2p.Install(nodes.Get(0), nodes.Get(1));
    p2p.Install(nodes.Get(1), nodes.Get(2));

    // Install NDN stack on all nodes
    ndn::StackHelper ndnHelper;
    ndnHelper.SetDefaultRoutes(true);
    ndnHelper.InstallAll();

    // Installing applications

    // Consumer
    ndn::AppHelper consumerHelper("ns3::ndn::WikiClient");
    consumerHelper.SetPrefix("/prefix");
    consumerHelper.SetAttribute("Frequency", StringValue("1")); // 1 interest a second
    consumerHelper.Install(nodes.Get(0));                        // first node

    // Producer
    ndn::AppHelper producerHelper("ns3::ndn::WikiProducer");
    // Producer will reply to all requests starting with /prefix
    producerHelper.SetPrefix("/prefix");
    producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
    producerHelper.Install(nodes.Get(2)); // last node

    Simulator::Stop(Seconds(20.0));

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
