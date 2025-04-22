#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NetAnimWirelessUDPFlow");

int main(int argc, char* argv[])
{
    // Log'ları aç
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    // NetAnim'de paketlerin görünmesi için zorunlu
    Packet::EnablePrinting();
    Packet::EnableChecking();

    // 1. Düğümler
    NodeContainer wifiStaNode;
    wifiStaNode.Create(1);

    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    // 2. Fiziksel katman
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns3-wifi-ssid");

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDevices = wifi.Install(phy, mac, wifiStaNode);

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevices = wifi.Install(phy, mac, wifiApNode);

    // 3. Mobilite
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    wifiApNode.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(50.0, 50.0, 0.0));

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(0, 100, 0, 100)),
                              "Speed", StringValue("ns3::ConstantRandomVariable[Constant=5]"));
    mobility.Install(wifiStaNode);
    wifiStaNode.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(30.0, 30.0, 0.0));

    // 4. Internet katmanı
    InternetStackHelper stack;
    stack.Install(wifiApNode);
    stack.Install(wifiStaNode);

    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer staInterface = address.Assign(staDevices);
    Ipv4InterfaceContainer apInterface = address.Assign(apDevices);

    // 5. Uygulamalar
    UdpEchoServerHelper echoServer(9);
    ApplicationContainer serverApps = echoServer.Install(wifiApNode.Get(0));
    serverApps.Start(Seconds(1.0));
    serverApps.Stop(Seconds(15.0));

    UdpEchoClientHelper echoClient(apInterface.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(5));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));
    ApplicationContainer clientApps = echoClient.Install(wifiStaNode.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(14.0));

    // 6. NetAnim ayarları
    AnimationInterface anim("scratch/wireless-animation.xml");

    anim.EnablePacketMetadata(true);  // UDP paket detaylarını NetAnim'e aktar

    uint64_t routerResId = anim.AddResource("/usr/local/share/netanim/icons/router.png");
    uint64_t laptopResId = anim.AddResource("/usr/local/share/netanim/icons/laptop.png");

    anim.UpdateNodeDescription(wifiApNode.Get(0)->GetId(), "Access Point");
    anim.UpdateNodeColor(wifiApNode.Get(0)->GetId(), 255, 0, 0);
    anim.UpdateNodeImage(wifiApNode.Get(0)->GetId(), routerResId);

    anim.UpdateNodeDescription(wifiStaNode.Get(0)->GetId(), "Client");
    anim.UpdateNodeColor(wifiStaNode.Get(0)->GetId(), 0, 0, 255);
    anim.UpdateNodeImage(wifiStaNode.Get(0)->GetId(), laptopResId);

    // 7. Simülasyonu çalıştır
    Simulator::Stop(Seconds(15.0));
    phy.EnablePcap("scratch/wireless-trace", apDevices.Get(0));

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
