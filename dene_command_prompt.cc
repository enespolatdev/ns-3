#include "ns3/core-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("SimpleTrafficSimulation");

void ReportPosition(Ptr<Node> node)
{
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    Vector pos = mobility->GetPosition();
    std::cout << "Zaman: " << Simulator::Now().GetSeconds()
              << "s | Araç " << node->GetId()
              << " konumu: x=" << pos.x << " y=" << pos.y << std::endl;

    // Tekrar planla
    Simulator::Schedule(Seconds(1.0), &ReportPosition, node);
}

int main(int argc, char *argv[])
{
    uint32_t numberOfVehicles = 3;
    double simulationTime = 10.0;
    double speed = 10.0; // m/s (36 km/h)

    CommandLine cmd;
    cmd.AddValue("n", "Araç sayısı", numberOfVehicles);
    cmd.AddValue("t", "Simülasyon süresi (saniye)", simulationTime);
    cmd.AddValue("speed", "Araç hızı (m/s)", speed);
    cmd.Parse(argc, argv);

    NodeContainer vehicles;
    vehicles.Create(numberOfVehicles);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(vehicles);

    for (uint32_t i = 0; i < numberOfVehicles; ++i)
    {
        Ptr<ConstantVelocityMobilityModel> mob = vehicles.Get(i)->GetObject<ConstantVelocityMobilityModel>();
        mob->SetPosition(Vector(0.0, i * 5.0, 0.0)); // Araçları y ekseninde 5 metre aralıklı yerleştir
        mob->SetVelocity(Vector(speed, 0.0, 0.0));   // Sabit hızla x yönünde hareket
        Simulator::Schedule(Seconds(1.0), &ReportPosition, vehicles.Get(i)); // Her saniye konum raporu başlat
    }

    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
