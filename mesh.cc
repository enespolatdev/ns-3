

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h" //NetAnim için gerekli header
#include "ns3/mobility-module.h" //Mobility için gerekli header



using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MeshScriptExample");

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS);
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
    LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

    //CREATING NODES AND GROUPING THEM ACCORDING TO THE RING TOPOLOGY
    NodeContainer nodes, node01, node12, node23, node30, node02, node13;
    nodes.Create(4);
    node01.Add(nodes.Get(0));
    node01.Add(nodes.Get(1));

    node12.Add(nodes.Get(1));
    node12.Add(nodes.Get(2));

    node23.Add(nodes.Get(2));
    node23.Add(nodes.Get(3));

    node30.Add(nodes.Get(3));
    node30.Add(nodes.Get(0));

    node02.Add(nodes.Get(2));
    node02.Add(nodes.Get(0));

    node13.Add(nodes.Get(1));
    node13.Add(nodes.Get(3));



    //CREATING POINT TO POINT LINKS BETWEEN NODES
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    //MAKING DEVICES ..
    NetDeviceContainer devices01, devices12, devices23, devices30, devices02, devices13;
    devices01 = pointToPoint.Install(node01);
    devices12 = pointToPoint.Install(node12);
    devices23 = pointToPoint.Install(node23);
    devices30 = pointToPoint.Install(node30);
    devices02 = pointToPoint.Install(node02);
    devices13 = pointToPoint.Install(node13);

    //INSTALLING IP STACK
    InternetStackHelper stack;
    stack.Install(nodes);

    //ASSIGNING IP ADDRESSES
    Ipv4AddressHelper address;
    
    address.SetBase("54.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer interfaces01 = address.Assign(devices01);

    address.SetBase("55.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer interfaces12 = address.Assign(devices12);

    address.SetBase("56.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer interfaces23 = address.Assign(devices23);

    address.SetBase("57.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer interfaces30 = address.Assign(devices30);

    address.SetBase("58.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer interfaces02 = address.Assign(devices02);

    address.SetBase("59.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer interfaces13 = address.Assign(devices13);

    //CREATING SERVERS
    UdpEchoServerHelper echoServer(9);

    ApplicationContainer serverApps = echoServer.Install(nodes.Get(0));
    serverApps.Start(Seconds(2.0));
    serverApps.Stop(Seconds(10.0));

    //CREATING CLIENTS
    UdpEchoClientHelper echoClient(interfaces01.GetAddress(0), 9);
    echoClient.SetAttribute("MaxPackets", UintegerValue(5));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(1.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = echoClient.Install(nodes.Get(2));
    clientApps.Start(Seconds(1.0));
    clientApps.Stop(Seconds(10.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    AnimationInterface anim("mesh.xml");
    pointToPoint.EnablePcapAll("mesh");
    

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
