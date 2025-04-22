#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WirelessMultiDeviceSimulation");

int main(int argc, char* argv[])
{
    // Log seviyelerini ayarla
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // Paket izleme için gerekli ayarlar
    Packet::EnablePrinting();
    Packet::EnableChecking();

    // 1. Düğümleri oluştur
    NodeContainer accessPoint;
    accessPoint.Create(1); // 1 adet erişim noktası

    NodeContainer laptops;
    laptops.Create(2); // 2 adet laptop

    NodeContainer smartphones;
    smartphones.Create(2); // 2 adet smartphone

    // Tüm istasyonları birleştir
    NodeContainer allStations;
    allStations.Add(laptops);
    allStations.Add(smartphones);

    // 2. Kablosuz aygıtları yapılandır
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue("HtMcs7"),
                                "ControlMode", StringValue("HtMcs0"));

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns3-wifi-network");

    // Erişim noktası yapılandırması
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevice = wifi.Install(phy, mac, accessPoint);

    // İstemci aygıtları yapılandırma
    mac.SetType("ns3::StaWifiMac", 
               "Ssid", SsidValue(ssid),
               "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevices = wifi.Install(phy, mac, allStations);

    // 3. Mobilite modellerini ayarla
    MobilityHelper mobility;

    // Erişim noktasını sabitle
    Ptr<ListPositionAllocator> apPosition = CreateObject<ListPositionAllocator>();
    apPosition->Add(Vector(50.0, 50.0, 0.0)); // Merkezde
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(apPosition);
    mobility.Install(accessPoint);

    // Cihazlar için rastgele hareket modeli
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                            "Bounds", RectangleValue(Rectangle(0, 100, 0, 100)),
                            "Speed", StringValue("ns3::UniformRandomVariable[Min=1|Max=3]"),
                            "Distance", DoubleValue(5.0));

    // Başlangıç pozisyonlarını dağıt
    Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable>();
    for (uint32_t i = 0; i < allStations.GetN(); ++i) {
        mobility.SetPositionAllocator("ns3::RandomRectanglePositionAllocator",
                                     "X", StringValue("ns3::UniformRandomVariable[Min=20|Max=80]"),
                                     "Y", StringValue("ns3::UniformRandomVariable[Min=20|Max=80]"));
        mobility.Install(allStations.Get(i));
    }

    // 4. İnternet yığınını kur
    InternetStackHelper stack;
    stack.Install(accessPoint);
    stack.Install(allStations);

    // IP adreslerini ata
    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer apInterface = address.Assign(apDevice);
    Ipv4InterfaceContainer staInterfaces = address.Assign(staDevices);

    // 5. Uygulamaları yapılandır
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(accessPoint.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(20.0));

    // Tüm istemciler için UDP istemcisi oluştur
    for (uint32_t i = 0; i < allStations.GetN(); ++i) {
        UdpEchoClientHelper echoClient(apInterface.GetAddress(0), 9);
        echoClient.SetAttribute("MaxPackets", UintegerValue(10));
        echoClient.SetAttribute("Interval", TimeValue(Seconds(0.5 + i*0.2))); // Farklı aralıklarla
        echoClient.SetAttribute("PacketSize", UintegerValue(512));
        
        // Hatalı satır çıkarıldı
        // echoClient.SetAttribute("TraceEnable", BooleanValue(true));

        ApplicationContainer clientApp = echoClient.Install(allStations.Get(i));
        clientApp.Start(Seconds(2.0 + i*0.5));
        clientApp.Stop(Seconds(18.0));
    }

    // 6. NetAnim görselleştirme ayarları
    AnimationInterface anim("wireless-simulation.xml");
    anim.EnablePacketMetadata(true);
    anim.EnableIpv4RouteTracking("routing-table.xml", Seconds(0), Seconds(20), Seconds(0.1));
    
    // Paket izleme ayarları
    anim.EnablePacketMetadata(true);
    anim.SetMobilityPollInterval(Seconds(0.1));
    anim.SetConstantPosition(accessPoint.Get(0), 50.0, 50.0, 0.0);

    // Arka plan ve ikon yollarını ayarla
    std::string basePath = "/usr/local/share/netanim/icons/";
    std::string routerPath = basePath + "router.png";
    std::string laptopPath = basePath + "laptop.png";
    std::string phonePath = basePath + "smartphone.png";
 

    // İkon kaynaklarını yükle
    uint64_t apIcon = anim.AddResource(routerPath);
    uint64_t laptopIcon = anim.AddResource(laptopPath);
    uint64_t phoneIcon = anim.AddResource(phonePath);
   


    // Erişim noktasını yapılandır
    uint32_t apNodeId = accessPoint.Get(0)->GetId();
    anim.UpdateNodeDescription(apNodeId, "AP");
    anim.UpdateNodeColor(apNodeId, 255, 0, 0);
    anim.UpdateNodeImage(apNodeId, apIcon);
    anim.UpdateNodeSize(apNodeId, 8, 8);

    // Laptopları yapılandır
    for (uint32_t i = 0; i < laptops.GetN(); ++i) {
        uint32_t nodeId = laptops.Get(i)->GetId();
        std::string nodeName = "Laptop " + std::to_string(i+1);
        anim.UpdateNodeDescription(nodeId, nodeName);
        anim.UpdateNodeColor(nodeId, 0, 0, 255);
        anim.UpdateNodeImage(nodeId, laptopIcon);
        anim.UpdateNodeSize(nodeId, 7, 7);
    }

    // Smartphone'ları yapılandır
    for (uint32_t i = 0; i < smartphones.GetN(); ++i) {
        uint32_t nodeId = smartphones.Get(i)->GetId();
        std::string nodeName = "Phone " + std::to_string(i+1);
        anim.UpdateNodeDescription(nodeId, nodeName);
        anim.UpdateNodeColor(nodeId, 0, 255, 0);
        anim.UpdateNodeImage(nodeId, phoneIcon);
        anim.UpdateNodeSize(nodeId, 6, 6);
    }

    // Paket izleme için gerekli ayarlar
    Config::SetDefault("ns3::WifiNetDevice::Mtu", UintegerValue(1500));

    // 7. Simülasyonu başlat
    Simulator::Stop(Seconds(20.0));
    phy.EnablePcapAll("wireless-trace");

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
