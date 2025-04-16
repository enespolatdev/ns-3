/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

 #include "ns3/applications-module.h"
 #include "ns3/core-module.h"
 #include "ns3/csma-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/ipv4-static-routing-helper.h"
 #include "ns3/network-module.h"
 #include "ns3/point-to-point-module.h"
 #include <cassert>
 #include <fstream>
 #include <iostream>
 #include <string>
 #include "ns3/netanim-module.h"
 
 using namespace ns3;
 
 NS_LOG_COMPONENT_DEFINE("StaticRoutingSlash32Test");
 
 int main(int argc, char* argv[])
 {
     LogComponentEnable("StaticRoutingSlash32Test", LOG_LEVEL_INFO);
     LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);
     LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
     LogComponentEnable("Ipv4StaticRouting", LOG_LEVEL_INFO);
 
     CommandLine cmd(__FILE__);
     cmd.Parse(argc, argv);
 
     Ptr<Node> nA = CreateObject<Node>();
     Ptr<Node> nB = CreateObject<Node>();
     Ptr<Node> nC = CreateObject<Node>();
 
     NodeContainer c = NodeContainer(nA, nB, nC);
 
     InternetStackHelper internet;
     internet.Install(c);
 
     // Point-to-point links
     NodeContainer nAnB = NodeContainer(nA, nB);
     NodeContainer nBnC = NodeContainer(nB, nC);
 
     PointToPointHelper p2p;
     p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
     p2p.SetChannelAttribute("Delay", StringValue("2ms"));
     NetDeviceContainer dAdB = p2p.Install(nAnB);
     NetDeviceContainer dBdC = p2p.Install(nBnC);
 
     // CSMA devices for /32 addresses
     Ptr<CsmaNetDevice> deviceA = CreateObject<CsmaNetDevice>();
     deviceA->SetAddress(Mac48Address::Allocate());
     nA->AddDevice(deviceA);
     deviceA->SetQueue(CreateObject<DropTailQueue<Packet>>());
 
     Ptr<CsmaNetDevice> deviceC = CreateObject<CsmaNetDevice>();
     deviceC->SetAddress(Mac48Address::Allocate());
     nC->AddDevice(deviceC);
     deviceC->SetQueue(CreateObject<DropTailQueue<Packet>>());
 
     // IP addressing
     Ipv4AddressHelper ipv4;
     ipv4.SetBase("10.1.1.0", "255.255.255.252");
     Ipv4InterfaceContainer iAiB = ipv4.Assign(dAdB); // A-B: 10.1.1.0/30
 
     ipv4.SetBase("10.1.1.4", "255.255.255.252");
     Ipv4InterfaceContainer iBiC = ipv4.Assign(dBdC); // B-C: 10.1.1.4/30
 
     // /32 addresses
     Ptr<Ipv4> ipv4A = nA->GetObject<Ipv4>();
     Ptr<Ipv4> ipv4B = nB->GetObject<Ipv4>();
     Ptr<Ipv4> ipv4C = nC->GetObject<Ipv4>();
 
     int32_t ifIndexA = ipv4A->AddInterface(deviceA);
     int32_t ifIndexC = ipv4C->AddInterface(deviceC);
 
     Ipv4InterfaceAddress ifInAddrA = Ipv4InterfaceAddress(Ipv4Address("172.16.1.1"), Ipv4Mask("/32"));
     ipv4A->AddAddress(ifIndexA, ifInAddrA);
     ipv4A->SetMetric(ifIndexA, 1);
     ipv4A->SetUp(ifIndexA);
 
     Ipv4InterfaceAddress ifInAddrC = Ipv4InterfaceAddress(Ipv4Address("192.168.1.1"), Ipv4Mask("/32"));
     ipv4C->AddAddress(ifIndexC, ifInAddrC);
     ipv4C->SetMetric(ifIndexC, 1);
     ipv4C->SetUp(ifIndexC);
 
     // =======================================================================
     // !!! NEW: Two-way static routing
     // =======================================================================
     Ipv4StaticRoutingHelper ipv4RoutingHelper;
 
     // A'dan C'ye route
     Ptr<Ipv4StaticRouting> staticRoutingA = ipv4RoutingHelper.GetStaticRouting(ipv4A);
     staticRoutingA->AddHostRouteTo(Ipv4Address("192.168.1.1"), Ipv4Address("10.1.1.2"), 1); // A -> B -> C
 
     // B'den C'ye route
     Ptr<Ipv4StaticRouting> staticRoutingB = ipv4RoutingHelper.GetStaticRouting(ipv4B);
     staticRoutingB->AddHostRouteTo(Ipv4Address("192.168.1.1"), Ipv4Address("10.1.1.6"), 2); // B -> C
 
     // !!! NEW: C'den A'ya route
     Ptr<Ipv4StaticRouting> staticRoutingC = ipv4RoutingHelper.GetStaticRouting(ipv4C);
     staticRoutingC->AddHostRouteTo(Ipv4Address("172.16.1.1"), Ipv4Address("10.1.1.5"), 1); // C -> B -> A
 
     // !!! NEW: B'den A'ya route
     staticRoutingB->AddHostRouteTo(Ipv4Address("172.16.1.1"), Ipv4Address("10.1.1.1"), 1); // B -> A
 
     // =======================================================================
     // !!! NEW: Two-way applications
     // =======================================================================
     uint16_t port = 9;
 
     // A -> C trafiği
     OnOffHelper onoffAC("ns3::UdpSocketFactory", Address(InetSocketAddress(ifInAddrC.GetLocal(), port)));
     onoffAC.SetConstantRate(DataRate(6000));
     ApplicationContainer appsAC = onoffAC.Install(nA);
     appsAC.Start(Seconds(1.0));
     appsAC.Stop(Seconds(10.0));
 
     // !!! NEW: C -> A trafiği
     OnOffHelper onoffCA("ns3::UdpSocketFactory", Address(InetSocketAddress(ifInAddrA.GetLocal(), port)));
     onoffCA.SetConstantRate(DataRate(6000));
     ApplicationContainer appsCA = onoffCA.Install(nC);
     appsCA.Start(Seconds(1.0));
     appsCA.Stop(Seconds(10.0));
 
     // Packet Sink'ler
     PacketSinkHelper sinkAC("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
     appsAC = sinkAC.Install(nC); // C'de A'dan gelenleri dinle
 
     // !!! NEW: A'da C'den gelenleri dinle
     PacketSinkHelper sinkCA("ns3::UdpSocketFactory", Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
     appsCA = sinkCA.Install(nA); 
     appsCA.Start(Seconds(1.0));
     appsCA.Stop(Seconds(10.0));
 
     // =======================================================================
     // Tracing ve Animasyon
     // =======================================================================
     AsciiTraceHelper ascii;
     p2p.EnableAsciiAll(ascii.CreateFileStream("./scratch/results/static-routing-slash32_abc.tr"));
     p2p.EnablePcapAll("./scratch/results/static-routing-slash32_abc");
 
     AnimationInterface anim("./scratch/results/static-routing-slash32_abc.xml");
     anim.SetConstantPosition(nA, 10.0, 10.0);
     anim.SetConstantPosition(nB, 20.0, 20.0);
     anim.SetConstantPosition(nC, 10.0, 30.0);
 
     Simulator::Run();
     Simulator::Destroy();
 
     return 0;
 }