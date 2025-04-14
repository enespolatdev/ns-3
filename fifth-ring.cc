/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "tutorial-app.h"

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/csma-module.h"
#include "ns3/wifi-standards.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FifthScriptExample2");

// Congestion window değişimi callback
static void
CwndChange(uint32_t oldCwnd, uint32_t newCwnd)
{
    NS_LOG_UNCOND(Simulator::Now().GetSeconds() << "\t" << newCwnd);
}

// Paket düşme callback
static void
RxDrop(Ptr<const Packet> p)
{
    NS_LOG_UNCOND("RxDrop at " << Simulator::Now().GetSeconds());
}

int
main(int argc, char* argv[])
{
    std::string mAlgo = "TcpCubic";
    uint32_t mTime= 20;
    CommandLine cmd(__FILE__);
    cmd.AddValue("mAlgo", "Congestion control algorithm", mAlgo);
    cmd.AddValue("mTime", "Simulation time", mTime);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::"+mAlgo)); 
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(1));
    Config::SetDefault("ns3::TcpL4Protocol::RecoveryType",
                       TypeIdValue(TypeId::LookupByName("ns3::TcpClassicRecovery")));

    NodeContainer allNodes, nodes01, nodes12, nodes23, nodes30;
    allNodes.Create(4);

    nodes01.Add(allNodes.Get(0)); nodes01.Add(allNodes.Get(1));
    nodes12.Add(allNodes.Get(1)); nodes12.Add(allNodes.Get(2));
    nodes23.Add(allNodes.Get(2)); nodes23.Add(allNodes.Get(3));
    nodes30.Add(allNodes.Get(3)); nodes30.Add(allNodes.Get(0));

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices01, devices12, devices23, devices30;
    devices01 = pointToPoint.Install(nodes01);
    devices12 = pointToPoint.Install(nodes12);
    devices23 = pointToPoint.Install(nodes23);
    devices30 = pointToPoint.Install(nodes30);

    Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
    em->SetAttribute("ErrorRate", DoubleValue(0.00001));
    devices01.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));
    devices12.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

    InternetStackHelper stack;
    stack.Install(allNodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.252");
    Ipv4InterfaceContainer interfaces01 = address.Assign(devices01);

    address.SetBase("10.1.2.0", "255.255.255.252");
    Ipv4InterfaceContainer interfaces12 = address.Assign(devices12);

    address.SetBase("10.1.3.0", "255.255.255.252");
    Ipv4InterfaceContainer interfaces23 = address.Assign(devices23);

    address.SetBase("10.1.4.0", "255.255.255.252");
    Ipv4InterfaceContainer interfaces30 = address.Assign(devices30);

    uint16_t sinkPort = 8080;
    Address sinkAddress(InetSocketAddress(interfaces01.GetAddress(1), sinkPort));

    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory",
                                      InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install(allNodes.Get(1));
    sinkApps.Start(Seconds(0.));
    sinkApps.Stop(Seconds(mTime));

    Ptr<Socket> ns3TcpSocket = Socket::CreateSocket(allNodes.Get(0), TcpSocketFactory::GetTypeId());
    ns3TcpSocket->TraceConnectWithoutContext("CongestionWindow", MakeCallback(&CwndChange));

    Ptr<TutorialApp> app = CreateObject<TutorialApp>();
    app->Setup(ns3TcpSocket, sinkAddress, 1040, 1000, DataRate("1Mbps"));
    allNodes.Get(0)->AddApplication(app);
    app->SetStartTime(Seconds(1.));
    app->SetStopTime(Seconds(mTime));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(mTime));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
