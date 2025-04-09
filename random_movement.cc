 #include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/wifi-standards.h"
#include "ns3/netanim-module.h"
 
using namespace ns3;
 
NS_LOG_COMPONENT_DEFINE("CustomExample");
 
int
main(int argc, char* argv[])
{
CommandLine cmd(__FILE__);
cmd.Parse(argc, argv);
 
Time::SetResolution(Time::NS);
LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
 
// step-1 = creating group of nodes....
NodeContainer allNodes,wifiStaNodes_test_link_0, wifiApNodes_test_link_0;
allNodes.Create(4);
 
wifiApNodes_test_link_0.Add(allNodes.Get(0));
wifiStaNodes_test_link_0.Add(allNodes.Get(1));
wifiStaNodes_test_link_0.Add(allNodes.Get(2));
wifiStaNodes_test_link_0.Add(allNodes.Get(3));
 
 
// step-2 = create link
CsmaHelper homeLan;
homeLan.SetChannelAttribute("DataRate", StringValue("100Mbps"));
homeLan.SetChannelAttribute("Delay", TimeValue(NanoSeconds(6560))); 
 
// step-3 = creating devices
NetDeviceContainer csmaDevices;
csmaDevices = homeLan.Install(allNodes);
 
MobilityHelper m;
Ptr<ListPositionAllocator> p = CreateObject<ListPositionAllocator>();
p->Add(Vector(10,20,0));
p->Add(Vector(20,20,0));
p->Add(Vector(20,10,0));
p->Add(Vector(10,10,0));
m.SetPositionAllocator(p);

// Random Way Point Mobility = Paused Randmized Change
m.SetMobilityModel("ns3::RandomWaypointMobilityModel",
			"Speed",StringValue("ns3::ConstantRandomVariable[Constant=1.0]"),
			"Pause",StringValue("ns3::ConstantRandomVariable[Constant=2.0]"),
			"PositionAllocator",PointerValue(p));
m.Install(allNodes.Get(1));

// Random Walk Mobility = Constant Randomize Change
m.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
		"Bounds",RectangleValue(Rectangle(-500,500,-500,500)));

m.Install(allNodes.Get(2));
m.Install(allNodes.Get(3));

m.SetMobilityModel("ns3::ConstantPositionMobilityModel");
m.Install(allNodes.Get(0));
 
// step-4 = Install ip stack
InternetStackHelper stack;
stack.Install(allNodes);
 
// step-5 = Assignment of IP Address
Ipv4AddressHelper address;
 
address.SetBase("54.0.0.0","255.0.0.0");
Ipv4InterfaceContainer interfacesHomeLan = address.Assign(csmaDevices); 
 
// step-6 = server configuration
UdpEchoServerHelper echoServer(54);
 
ApplicationContainer serverApps = echoServer.Install(allNodes.Get(1));
serverApps.Start(Seconds(1.0));
serverApps.Stop(Seconds(11.0));
 
// step-7 = client configuration
UdpEchoClientHelper echoClient(interfacesHomeLan.GetAddress(1),54);
echoClient.SetAttribute("MaxPackets", UintegerValue(2));
echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
echoClient.SetAttribute("PacketSize", UintegerValue(1024));
 
ApplicationContainer clientApps = echoClient.Install(allNodes.Get(3));
clientApps.Start(Seconds(2.0));
clientApps.Stop(Seconds(11.0));
 
Ipv4GlobalRoutingHelper::PopulateRoutingTables();
AnimationInterface anim("a_random_mowement.xml");
 
 
Simulator::Stop(Seconds(15)); 
Simulator::Run();
Simulator::Destroy();
return 0;
}