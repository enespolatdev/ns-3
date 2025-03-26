

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h" //NetAnim için gerekli header
#include "ns3/mobility-module.h" //Mobility için gerekli header



using namespace ns3;

NS_LOG_COMPONENT_DEFINE("StarScriptExample");

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS);
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    //CREATING NODES AND GROUPING THEM ACCORDING TO THE RING TOPOLOGY
    NodeContainer nodes, node01, node02, node03, node04;
    nodes.Create(4);
    node01.Add(nodes.Get(0));
    node01.Add(nodes.Get(1));

    node02.Add(nodes.Get(0));
    node02.Add(nodes.Get(2));

    node03.Add(nodes.Get(0));
    node03.Add(nodes.Get(3));

    node04.Add(nodes.Get(0));
    node04.Add(nodes.Get(4));



    //CREATING POINT TO POINT LINKS BETWEEN NODES
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    //MAKING DEVICES ..
    NetDeviceContainer devices01, devices02, devices03, devices04;
    devices01 = pointToPoint.Install(node01);
    devices02 = pointToPoint.Install(node02);
    devices03 = pointToPoint.Install(node03);
    devices04 = pointToPoint.Install(node04);
   

    //INSTALLING IP STACK
    InternetStackHelper stack;
    stack.Install(nodes);

    //ASSIGNING IP ADDRESSES
    Ipv4AddressHelper address;
    
    address.SetBase("54.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer interfaces01 = address.Assign(devices01);

    address.SetBase("55.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer interfaces02 = address.Assign(devices02);

    address.SetBase("56.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer interfaces03 = address.Assign(devices03);

    address.SetBase("57.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer interfaces04 = address.Assign(devices04);

    

    //CREATING SERVERS
    UdpEchoServerHelper echoServer(9);

    ApplicationContainer serverApps = echoServer.Install(nodes.Get(1));
    serverApps.Start(Seconds(2.0));
    serverApps.Stop(Seconds(10.0));

    //CREATING CLIENTS
    UdpEchoClientHelper echoClient(interfaces01.GetAddress(1), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(5));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(nodes.Get(3));
    clientApps.Start(Seconds(1.0));
    clientApps.Stop(Seconds(10.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    AnimationInterface anim("star.xml");
    pointToPoint.EnablePcapAll("star");
    

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
