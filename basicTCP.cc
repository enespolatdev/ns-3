#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/netanim-module.h" //NetAnim için gerekli header

// Default Network Topology
//
//       54.0.0.0/8
// n0 -------------- n1
//    point-to-point
//  TCP 1Mbps

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("BasicTcpExample");

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    LogComponentEnable("OnOffApplication", LOG_LEVEL_INFO);

    NodeContainer allNodes, nodes01;
    allNodes.Create(2);
    
    nodes01.Add(allNodes.Get(0));
    nodes01.Add(allNodes.Get(1));

    PointToPointHelper link;
    link.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
    link.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices01;
    devices01 = link.Install(nodes01);

    InternetStackHelper stack;
    stack.Install(allNodes);

    Ipv4AddressHelper address;
    address.SetBase("54.0.0.0", "255.0.0.0");

    Ipv4InterfaceContainer interfaces01 = address.Assign(devices01);

    //Server Aplikasyonu
    uint16_t Port = 80;
    Address sinkAddress(InetSocketAddress(interfaces01.GetAddress(1), Port));
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkAddress);
    
    ApplicationContainer sinkApp = sinkHelper.Install(nodes01.Get(1));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(10.0));

    //Client Aplikasyonu
    OnOffHelper onOffHelper("ns3::TcpSocketFactory", sinkAddress);
    onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    onOffHelper.SetAttribute("DataRate", StringValue("1Mbps"));
    onOffHelper.SetAttribute("PacketSize", UintegerValue(1500));

    ApplicationContainer clientApp = onOffHelper.Install(nodes01.Get(0));
    clientApp.Start(Seconds(1.0));
    clientApp.Stop(Seconds(10.0));

    AnimationInterface anim("BasicTCP.xml");
    AnimationInterface::SetConstantPosition(allNodes.Get(0), 10, 20);
    AnimationInterface::SetConstantPosition(allNodes.Get(1), 30, 20);
    link.EnablePcapAll("BasicTCP");
    

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
