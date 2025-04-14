/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

 #include "tutorial-app.h"
 #include "ns3/applications-module.h"
 #include "ns3/core-module.h"
 #include "ns3/internet-module.h"
 #include "ns3/network-module.h"
 #include "ns3/point-to-point-module.h"
 #include <fstream>
 #include <vector>
 
 using namespace ns3;
 
 NS_LOG_COMPONENT_DEFINE("TCP-Congestion-Example");
 
 // Congestion window change callback
 static void CwndChange(uint32_t oldCwnd, uint32_t newCwnd)
 {
     NS_LOG_UNCOND(Simulator::Now().GetSeconds() << "\t" << newCwnd);
 }
 
 int main(int argc, char* argv[])
 {
     // Parametreler ve varsayılan değerler
     std::string congestionAlgorithm = "TcpCubic";
     uint32_t simulationTime = 20;
     int nodeCount = 3;
     
     CommandLine cmd(__FILE__);
     cmd.AddValue("mAlgo", "Congestion control algorithm", congestionAlgorithm);
     cmd.AddValue("mTime", "Simulation time (seconds)", simulationTime);
     cmd.AddValue("mNodes", "Number of nodes (minimum 2)", nodeCount);
     cmd.Parse(argc, argv);
 
     // Minimum düğüm kontrolü
     if (nodeCount < 2)
     {
         NS_FATAL_ERROR("Number of nodes must be at least 2");
     }
 
     // TCP Konfigürasyonu
     Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::" + congestionAlgorithm));
     Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(1));
     Config::SetDefault("ns3::TcpL4Protocol::RecoveryType",
                       TypeIdValue(TypeId::LookupByName("ns3::TcpClassicRecovery")));
 
     // Ağ Topolojisi Oluşturma
     NodeContainer allNodes;
     allNodes.Create(nodeCount);
 
     // Point-to-Point Bağlantılar
     std::vector<NodeContainer> nodeGroups;
     for(int i = 0; i < nodeCount - 1; i++)
     {
         NodeContainer nc(allNodes.Get(i), allNodes.Get(i + 1));
         nodeGroups.push_back(nc);
     }
 
     // Ağ Cihazları Konfigürasyonu
     PointToPointHelper p2p;
     p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
     p2p.SetChannelAttribute("Delay", StringValue("2ms"));
 
     // Hata Modeli
     Ptr<RateErrorModel> errorModel = CreateObject<RateErrorModel>();
     errorModel->SetAttribute("ErrorRate", DoubleValue(0.00001));
 
     std::vector<NetDeviceContainer> deviceGroups;
     for(auto& group : nodeGroups)
     {
         NetDeviceContainer devices = p2p.Install(group);
         devices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(errorModel));
         deviceGroups.push_back(devices);
     }
 
     // Internet Stack Kurulumu
     InternetStackHelper stack;
     stack.Install(allNodes);
 
     // IP Adresleme
     Ipv4AddressHelper address;
     std::vector<Ipv4InterfaceContainer> interfaceGroups;
     uint32_t baseNetwork = 1; // 10.1.x.0 networkleri için
     
     for(auto& devices : deviceGroups)
     {
         std::ostringstream networkAddress;
         networkAddress << "10.1." << baseNetwork << ".0";
         address.SetBase(networkAddress.str().c_str(), "255.255.255.252");
         interfaceGroups.push_back(address.Assign(devices));
         baseNetwork++;
     }
 
     // Uygulama Katmanı
     // Sunucu (Sink) Konfigürasyonu
     uint16_t port = 8080;
     Address sinkAddress(InetSocketAddress(interfaceGroups.back().GetAddress(1), port));
     
     PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
     ApplicationContainer sinkApps = sinkHelper.Install(allNodes.Get(nodeCount - 1));
     sinkApps.Start(Seconds(0.0));
     sinkApps.Stop(Seconds(simulationTime));
 
     // İstemci Konfigürasyonu
     Ptr<Socket> tcpSocket = Socket::CreateSocket(allNodes.Get(0), TcpSocketFactory::GetTypeId());
     tcpSocket->TraceConnectWithoutContext("CongestionWindow", MakeCallback(&CwndChange));
 
     Ptr<TutorialApp> app = CreateObject<TutorialApp>();
     app->Setup(tcpSocket, sinkAddress, 1040, 1000, DataRate("1Mbps"));
     allNodes.Get(0)->AddApplication(app);
     app->SetStartTime(Seconds(1.0));
     app->SetStopTime(Seconds(simulationTime));
 
     // Routing Tabloları
     Ipv4GlobalRoutingHelper::PopulateRoutingTables();
 
     // Simülasyon
     Simulator::Stop(Seconds(simulationTime));
     Simulator::Run();
     Simulator::Destroy();
 
     return 0;
 }