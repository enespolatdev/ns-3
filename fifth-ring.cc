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

/

/**
 * Congestion window change callback
 *
 * \param oldCwnd Old congestion window.
 * \param newCwnd New congestion window.
 */
static void
CwndChange(uint32_t oldCwnd, uint32_t newCwnd)
{
    NS_LOG_UNCOND(Simulator::Now().GetSeconds() << "\t" << newCwnd);
}

/**
 * Rx drop callback
 *
 * \param p The dropped packet.
 */
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
    //std::cout << "Congestion control algorithm: " << mAlgo << std::endl;

    // In the following three lines, TCP NewReno is used as the congestion
    // control algorithm, the initial congestion window of a TCP connection is
    // set to 1 packet, and the classic fast recovery algorithm is used. Note
    // that this configuration is used only to demonstrate how TCP parameters
    // can be configured in ns-3. Otherwise, it is recommended to use the default
    // settings of TCP in ns-3.
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::"+mAlgo)); 
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(1));
    Config::SetDefault("ns3::TcpL4Protocol::RecoveryType",
                       TypeIdValue(TypeId::LookupByName("ns3::TcpClassicRecovery")));

    NodeContainer allNodes, nodes01, nodes12, nodes23, nodes30;
    allNodes.Create(4);

    nodes01.Add(allNodes.Get(0));
    nodes01.Add(allNodes.Get(1));

    nodes12.Add(allNodes.Get(1));
    nodes12.Add(allNodes.Get(2));

    nodes23.Add(allNodes.Get(2));
    nodes23.Add(allNodes.Get(3));

    nodes30.Add(allNodes.Get(3));
    nodes30.Add(allNodes.Get(0));

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices01, devices02, devices03, devices04;
    devices01 = pointToPoint.Install(nodes01);
    devices02 = pointToPoint.Install(nodes12);
    devices03 = pointToPoint.Install(nodes23);
    devices04 = pointToPoint.Install(nodes30);
    

    Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
    em->SetAttribute("ErrorRate", DoubleValue(0.00001));
    devices01.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));
    devices02.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));
    //devices03.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));
    //devices04.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

    InternetStackHelper stack;
    stack.Install(allNodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.252");
    Ipv4InterfaceContainer interfaces01 = address.Assign(devices01);

    address.SetBase("10.1.2.0", "255.255.255.252");
    Ipv4InterfaceContainer interfaces02 = address.Assign(devices02);

    address.SetBase("10.1.3.0", "255.255.255.252");
    Ipv4InterfaceContainer interfaces03 = address.Assign(devices03);

    address.SetBase("10.1.4.0", "255.255.255.252");
    Ipv4InterfaceContainer interfaces04 = address.Assign(devices04);

    uint16_t sinkPort = 8080;
    Address sinkAddress(InetSocketAddress(interfaces01.GetAddress(1), sinkPort));

    // server
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory",
                                      InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install(allNodes.Get(1));
    sinkApps.Start(Seconds(0.));
    sinkApps.Stop(Seconds(mTime));

    //socket
    Ptr<Socket> ns3TcpSocket = Socket::CreateSocket(allNodes.Get(0), TcpSocketFactory::GetTypeId());
    ns3TcpSocket->TraceConnectWithoutContext("CongestionWindow", MakeCallback(&CwndChange));

    //client
    Ptr<TutorialApp> app = CreateObject<TutorialApp>();
    app->Setup(ns3TcpSocket, sinkAddress, 1040, 1000, DataRate("1Mbps"));
    allNodes.Get(0)->AddApplication(app);
    app->SetStartTime(Seconds(1.));
    app->SetStopTime(Seconds(mTime));

    //devices01.Get(1)->TraceConnectWithoutContext("PhyRxDrop", MakeCallback(&RxDrop));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Stop(Seconds(mTime));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
