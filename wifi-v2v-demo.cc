#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiV2VDemo");

// BSM (Basic Safety Message) uygulaması
class BsmApplication : public Application
{
public:
  static TypeId GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::BsmApplication")
      .SetParent<Application> ()
      .SetGroupName ("V2V")
      .AddConstructor<BsmApplication> ()
      ;
    return tid;
  }

  BsmApplication ()
  {
    m_socket = nullptr;
    m_peer = Address ();
    m_packetSize = 200;  // BSM paket boyutu (byte)
    m_interval = Seconds (0.1);  // BSM gönderim sıklığı (100ms)
  }

  ~BsmApplication() override
  {
    m_socket = nullptr;
  }

  void SetRemote (Address ip, uint16_t port)
  {
    m_peer = InetSocketAddress (Ipv4Address::ConvertFrom (ip), port);
  }

private:
  void StartApplication () override
  {
    if (!m_socket)
      {
        TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket (GetNode (), tid);
        m_socket->Bind ();
        // Broadcast paketlerini etkinleştir
        m_socket->SetAllowBroadcast (true);
      }
    m_socket->Connect (m_peer);
    SendBsm ();
  }

  void StopApplication () override
  {
    if (m_socket)
      {
        m_socket->Close ();
      }
  }

  void SendBsm ()
  {
    if (!m_socket)
      {
        return;
      }

    // BSM paketini oluştur
    auto mobility = GetNode ()->GetObject<MobilityModel> ();
    Vector pos = mobility->GetPosition ();
    Vector vel = mobility->GetVelocity ();

    std::ostringstream msg;
    msg << "BSM:"
        << " ID=" << GetNode ()->GetId ()
        << " POS=" << pos.x << "," << pos.y
        << " VEL=" << vel.x << "," << vel.y;

    Ptr<Packet> packet = Create<Packet> ((uint8_t*) msg.str().c_str(), m_packetSize);
    m_socket->Send (packet);

    // Log mesajı
    NS_LOG_INFO ("Node " << GetNode ()->GetId () 
                << " sent BSM at " << Simulator::Now ().GetSeconds () 
                << "s: " << msg.str());

    // Bir sonraki BSM'i planla
    Simulator::Schedule (m_interval, &BsmApplication::SendBsm, this);
  }

private:
  Ptr<Socket> m_socket;
  Address m_peer;
  uint32_t m_packetSize;
  Time m_interval;
};

// Özel mesajlaşma uygulaması
class PrivateMessageApplication : public Application
{
public:
  static TypeId GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::PrivateMessageApplication")
      .SetParent<Application> ()
      .SetGroupName ("V2V")
      .AddConstructor<PrivateMessageApplication> ()
      ;
    return tid;
  }

  PrivateMessageApplication ()
  {
    m_socket = nullptr;
    m_peer = Address ();
    m_packetSize = 100;
    m_interval = Seconds (1.0);  // Her saniye mesaj gönder
  }

  void SetRemote (Address ip, uint16_t port)
  {
    m_peer = InetSocketAddress (Ipv4Address::ConvertFrom (ip), port);
  }

private:
  void StartApplication () override
  {
    if (!m_socket)
      {
        TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket (GetNode (), tid);
        m_socket->Bind ();
      }
    m_socket->Connect (m_peer);
    SendPrivateMessage ();
  }

  void StopApplication () override
  {
    if (m_socket)
      {
        m_socket->Close ();
      }
  }

  void SendPrivateMessage ()
  {
    if (!m_socket)
      {
        return;
      }

    std::string msg = "Özel mesaj: Araç " + std::to_string(GetNode()->GetId()) + "'den";
    Ptr<Packet> packet = Create<Packet> ((uint8_t*) msg.c_str(), msg.length());
    m_socket->Send (packet);

    NS_LOG_INFO ("Araç " << GetNode ()->GetId () 
                << " özel mesaj gönderdi at " << Simulator::Now ().GetSeconds () 
                << "s: " << msg);

    Simulator::Schedule (m_interval, &PrivateMessageApplication::SendPrivateMessage, this);
  }

private:
  Ptr<Socket> m_socket;
  Address m_peer;
  uint32_t m_packetSize;
  Time m_interval;
};

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  // Logging'i etkinleştir
  LogComponentEnable ("WifiV2VDemo", LOG_LEVEL_INFO);

  // Düğümleri oluştur (5 araç)
  NodeContainer nodes;
  nodes.Create (5);

  // Hareket modelini oluştur
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  mobility.Install (nodes);

  // Araçların başlangıç pozisyonlarını ve hızlarını ayarla
  // Her araç farklı şeritte ve farklı hızda hareket ediyor
  double laneWidth = 4.0; // Şerit genişliği (metre)
  
  // Araç 1: En sol şeritte, en hızlı
  Ptr<ConstantVelocityMobilityModel> mov1 = nodes.Get (0)->GetObject<ConstantVelocityMobilityModel> ();
  mov1->SetPosition (Vector (0.0, 0.0, 0.0));
  mov1->SetVelocity (Vector (25.0, 0.0, 0.0));  // 90 km/h

  // Araç 2: İkinci şeritte
  Ptr<ConstantVelocityMobilityModel> mov2 = nodes.Get (1)->GetObject<ConstantVelocityMobilityModel> ();
  mov2->SetPosition (Vector (30.0, laneWidth, 0.0));
  mov2->SetVelocity (Vector (22.0, 0.0, 0.0));  // 79 km/h

  // Araç 3: Orta şeritte
  Ptr<ConstantVelocityMobilityModel> mov3 = nodes.Get (2)->GetObject<ConstantVelocityMobilityModel> ();
  mov3->SetPosition (Vector (60.0, 2 * laneWidth, 0.0));
  mov3->SetVelocity (Vector (19.0, 0.0, 0.0));  // 68 km/h

  // Araç 4: Dördüncü şeritte
  Ptr<ConstantVelocityMobilityModel> mov4 = nodes.Get (3)->GetObject<ConstantVelocityMobilityModel> ();
  mov4->SetPosition (Vector (90.0, 3 * laneWidth, 0.0));
  mov4->SetVelocity (Vector (17.0, 0.0, 0.0));  // 61 km/h

  // Araç 5: En sağ şeritte, en yavaş
  Ptr<ConstantVelocityMobilityModel> mov5 = nodes.Get (4)->GetObject<ConstantVelocityMobilityModel> ();
  mov5->SetPosition (Vector (120.0, 4 * laneWidth, 0.0));
  mov5->SetVelocity (Vector (15.0, 0.0, 0.0));  // 54 km/h

  // WiFi ayarlarını yapılandır
  WifiHelper wifi;
  wifi.SetStandard (WIFI_STANDARD_80211p);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                              "DataMode", StringValue ("OfdmRate6MbpsBW10MHz"),
                              "ControlMode", StringValue ("OfdmRate6MbpsBW10MHz"));

  // Kanal oluştur
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5.9e9));
  Ptr<YansWifiChannel> channel = wifiChannel.Create ();

  // WiFi cihazlarını oluştur
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");
  YansWifiPhyHelper wifiPhy;
  wifiPhy.SetChannel (channel);
  NetDeviceContainer devices = wifi.Install (wifiPhy, wifiMac, nodes);

  // Internet stack'i kur
  InternetStackHelper internet;
  internet.Install (nodes);

  // IP adreslerini ata
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer interfaces = ipv4.Assign (devices);

  // BSM uygulamalarını oluştur
  uint16_t port = 9;
  
  // Her düğüm için BSM uygulaması oluştur
  for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
      Ptr<BsmApplication> app = CreateObject<BsmApplication> ();
      app->SetRemote (Ipv4Address ("255.255.255.255"), port);
      nodes.Get(i)->AddApplication (app);
      app->SetStartTime (Seconds (1.0));
      app->SetStopTime (Seconds (20.0));
    }

  // Araç 0 ve Araç 1 arasında özel mesajlaşma kur
  uint16_t privatePort = 10;
  Ptr<PrivateMessageApplication> app0 = CreateObject<PrivateMessageApplication> ();
  app0->SetRemote (interfaces.GetAddress (1), privatePort);  // Araç 1'e gönder
  nodes.Get(0)->AddApplication (app0);
  app0->SetStartTime (Seconds (2.0));
  app0->SetStopTime (Seconds (20.0));

  Ptr<PrivateMessageApplication> app1 = CreateObject<PrivateMessageApplication> ();
  app1->SetRemote (interfaces.GetAddress (0), privatePort);  // Araç 0'a gönder
  nodes.Get(1)->AddApplication (app1);
  app1->SetStartTime (Seconds (2.0));
  app1->SetStopTime (Seconds (20.0));

  // Simülasyonu çalıştır
  Simulator::Stop (Seconds (21.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
} 