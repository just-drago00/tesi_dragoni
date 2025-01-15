/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/**
 * \ingroup examples
 * \file tesi.cc
 * \brief An project simulating NR V2V SL scenario with 6 sectors, where in each serctor 
 * there is one RSU and 6 vehicles. The vehicles are moving in a straight line with a
 * constant speed of  5 Km/h. The RSU is placed at the center of the sector. The vehicles
 * are sending data to the RSU using NR V2V SL communication in FR2 band, and the RSU is 
 * sending controlling data to the vehicles. In the scenario there are also 24 DO and 12 
 * UGV, sending packets to the gNB using FR1 bands. The gNB is placed at the center of the
 * scenario. The scenario is simulated for 1 second. The goal of this project is to evaluate
 * the performance of the NR V2V SL communication in a dense urban scenario using KPIs
 * as PRR, PIR, and throughput, delay.
 * 
 *
 * \code{.unparsed}
$ ./ns3 run "tesi --help"
    \endcode
 *
 */

#include "v2x-kpi.h"
#include <cmath>
#include "ns3/flow-monitor-module.h"
#include "ns3/antenna-module.h"
#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/log.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/nr-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/stats-module.h"
#include <random>
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("tesi");

/*
 * Global methods to hook trace sources from different layers of
 * the protocol stack.
 */

/**
 * \brief Method to listen the trace SlPscchScheduling of NrUeMac, which gets
 *        triggered upon the transmission of SCI format 1-A from UE MAC.
 *
 * \param pscchStats Pointer to the UeMacPscchTxOutputStats class,
 *        which is responsible to write the trace source parameters to a database.
 * \param pscchStatsParams Parameters of the trace source.
 */
void
NotifySlPscchScheduling(UeMacPscchTxOutputStats* pscchStats,
                        const SlPscchUeMacStatParameters pscchStatsParams)
{
    pscchStats->Save(pscchStatsParams);
}

/**
 * \brief Method to listen the trace SlPsschScheduling of NrUeMac, which gets
 *        triggered upon the transmission of SCI format 2-A and data from UE MAC.
 *
 * \param psschStats Pointer to the UeMacPsschTxOutputStats class,
 *        which is responsible to write the trace source parameters to a database.
 * \param psschStatsParams Parameters of the trace source.
 */
void
NotifySlPsschScheduling(UeMacPsschTxOutputStats* psschStats,
                        const SlPsschUeMacStatParameters psschStatsParams)
{
    psschStats->Save(psschStatsParams);
}

/**
 * \brief Method to listen the trace RxPscchTraceUe of NrSpectrumPhy, which gets
 *        triggered upon the reception of SCI format 1-A.
 *
 * \param pscchStats Pointer to the UePhyPscchRxOutputStats class,
 *        which is responsible to write the trace source parameters to a database.
 * \param pscchStatsParams Parameters of the trace source.
 */
void
NotifySlPscchRx(UePhyPscchRxOutputStats* pscchStats,
                const SlRxCtrlPacketTraceParams pscchStatsParams)
{
    pscchStats->Save(pscchStatsParams);
}

/**
 * \brief Method to listen the trace RxPsschTraceUe of NrSpectrumPhy, which gets
 *        triggered upon the reception of SCI format 2-A and data.
 *
 * \param psschStats Pointer to the UePhyPsschRxOutputStats class,
 *        which is responsible to write the trace source parameters to a database.
 * \param psschStatsParams Parameters of the trace source.
 */
void
NotifySlPsschRx(UePhyPsschRxOutputStats* psschStats,
                const SlRxDataPacketTraceParams psschStatsParams)
{
    psschStats->Save(psschStatsParams);
}

/**
 * \brief Method to listen the application level traces of type TxWithAddresses
 *        and RxWithAddresses.
 * \param stats Pointer to the UeToUePktTxRxOutputStats class,
 *        which is responsible to write the trace source parameters to a database.
 * \param node The pointer to the TX or RX node
 * \param localAddrs The local IPV4 address of the node
 * \param txRx The string indicating the type of node, i.e., TX or RX
 * \param p The packet
 * \param srcAddrs The source address from the trace
 * \param dstAddrs The destination address from the trace
 * \param seqTsSizeHeader The SeqTsSizeHeader
 */
void
UePacketTraceDb(UeToUePktTxRxOutputStats* stats,
                Ptr<Node> node,
                const Address& localAddrs,
                std::string txRx,
                Ptr<const Packet> p,
                const Address& srcAddrs,
                const Address& dstAddrs,
                const SeqTsSizeHeader& seqTsSizeHeader)
{
    uint32_t nodeId = node->GetId();
    uint64_t imsi = node->GetDevice(0)->GetObject<NrUeNetDevice>()->GetImsi();
    uint32_t seq = seqTsSizeHeader.GetSeq();
    uint32_t pktSize = p->GetSize() + seqTsSizeHeader.GetSerializedSize();

    stats->Save(txRx, localAddrs, nodeId, imsi, pktSize, srcAddrs, dstAddrs, seq);
}

/**
 * \brief Trace sink for RxRlcPduWithTxRnti trace of NrUeMac
 * \param stats Pointer to UeRlcRxOutputStats API responsible to write the
 *        information communicated by this trace into a database.
 * \param imsi The IMSI of the UE
 * \param rnti The RNTI of the UE
 * \param txRnti The RNTI of the TX UE
 * \param lcid The logical channel id
 * \param rxPduSize The received PDU size
 * \param delay The end-to-end, i.e., from TX RLC entity to RX
 *        RLC entity, delay in Seconds.
 */
void
NotifySlRlcPduRx(UeRlcRxOutputStats* stats,
                 uint64_t imsi,
                 uint16_t rnti,
                 uint16_t txRnti,
                 uint8_t lcid,
                 uint32_t rxPduSize,
                 double delay)
{
    stats->Save(imsi, rnti, txRnti, lcid, rxPduSize, delay);
}

/**
 * \brief Install highway mobility
 * \param numSector Total number of lanes
 * \param numCAVsPerSector number of vehicles per lane (only odd numbers)
 * \param interCAVsDist The distance between the vehicles in a same lane
 * \param interSectorDist The distance between the lanes, i.e., the distance
 *        between the vehicles of two different lanes
 * \param CAVsSpeed The CAVsSpeed of the vehicles
 * \return A node container containing the vehicle UEs with their mobility model
 *         installed
 */
NodeContainer
InstallSLMobility(uint16_t numSector, uint16_t numCAVsPerSector,  double CAVsSpeed)
{
    int degrees = 0;
    double radians = degrees * M_PI / 180.0;
    double radians1 = degrees * M_PI / 180.0;
    int flag = 0;
    int r = 4000;
    int r1 = 20;
   
    NodeContainer ueNodes;
    ueNodes.Create(numCAVsPerSector * numSector);

    std::cout << "Total SL UEs = " << ueNodes.GetN() << std::endl;

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");

    mobility.Install(ueNodes);
    
    for (int i = 0; i < numSector; i++)
    {
        if (flag % 2 == 0 ){
            ueNodes.Get(i * numCAVsPerSector)->GetObject<ConstantVelocityMobilityModel>()->SetPosition(Vector(r * cos(radians), r * sin(radians), 0.0));
            ueNodes.Get(i * numCAVsPerSector)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, 0.0, 0.0));
            for(int j = 1 ; j < numCAVsPerSector; j++ ){
                ueNodes.Get(i * numCAVsPerSector + j)->GetObject<ConstantVelocityMobilityModel>()->SetPosition(Vector(r * cos(radians) + r1 * cos(radians1), r * sin(radians) + r1 * sin(radians1), 0.0));
                ueNodes.Get(i * numCAVsPerSector + j)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(CAVsSpeed * cos(radians1), CAVsSpeed * sin(radians1), 0.0));
                radians1 = radians1 + (60 * M_PI / 180.0); 
            }
        }
        if  (flag % 2 == 1 ){
            ueNodes.Get(i * numCAVsPerSector)->GetObject<ConstantVelocityMobilityModel>()->SetPosition(Vector(r * cos(radians), r * sin(radians), 30.0));
            ueNodes.Get(i * numCAVsPerSector)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, 0.0, 0.0));
            for(int j = 1 ; j < numCAVsPerSector; j++ ){
                ueNodes.Get(i * numCAVsPerSector + j)->GetObject<ConstantVelocityMobilityModel>()->SetPosition(Vector(r * cos(radians) + r1 * cos(radians1), r * sin(radians) + r1 * sin(radians1), 0.0));
                ueNodes.Get(i * numCAVsPerSector + j)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(CAVsSpeed * cos(radians1), CAVsSpeed * sin(radians1), 0.0));
                radians1 = radians1 + (60 * M_PI / 180.0); 
            }
        } 
        radians = radians + (60 * M_PI / 180.0);
        flag++;
    }
    
    return ueNodes;
}
double generateRandomDouble(double min, double max) {
    std::random_device rd;  // Obtain a random number from hardware
    std::mt19937 gen(rd()); // Seed the generator
    std::uniform_real_distribution<> distr(min, max); // Define the range

    return distr(gen);
}
NodeContainer
InstallFR1Mobility(int numugv , double ugvspeed , int numoperators, double opspeed )
{
    int degrees = 0;
    double radians2 = degrees * M_PI / 180.0; 
    int r2 = 600;
    NodeContainer ueNodes;
    ueNodes.Create(numugv + numoperators + 1) ;
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(ueNodes);

    std::cout << "Total FR1 UEs = " << ueNodes.GetN() << std::endl;

    for(int i = 0; i < numugv; i++){
        ueNodes.Get(i)->GetObject<ConstantVelocityMobilityModel>()->SetPosition(Vector(r2 * cos(radians2), r2 * sin(radians2), 0.0));
        ueNodes.Get(i)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(ugvspeed * cos(radians2), ugvspeed * sin(radians2), 0.0));
        radians2 = radians2 + (30 * M_PI / 180.0);   
    }
    for (int i = numugv; i < numugv + numoperators; i++){
        double randomValue = generateRandomDouble(5.0, 100.0);
        double angle = generateRandomDouble(0.0, 360.0);
        double rad = angle * M_PI / 180.0;
        ueNodes.Get(i)->GetObject<ConstantVelocityMobilityModel>()->SetPosition(Vector(randomValue, randomValue, 0.0));
        ueNodes.Get(i)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(opspeed * cos(rad), opspeed * sin(rad), 0.0)); 
    }   
    ueNodes.Get(numugv + numoperators)->GetObject<ConstantVelocityMobilityModel>()->SetPosition(Vector(1.0, 1.0, 0.0));
    ueNodes.Get(numugv + numoperators)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, 0.0, 0.0));
    return ueNodes;
}


/**
 * \brief Write the gnuplot script to plot the initial positions of the vehicle UEs
 * \param posFilename The name of the file, which this script needs to read to plot positions
 */
void
WriteInitPosGnuScript(std::string posFilename)
{
    std::ofstream outFile;
    std::string filename = "gnu-script-" + posFilename;
    outFile.open(filename.c_str(), std::ios_base::out | std::ios_base::trunc);
    if (!outFile.is_open())
    {
        NS_LOG_ERROR("Can't open file " << filename);
        return;
    }

    std::string pngFileName;
    pngFileName = posFilename.substr(0, posFilename.rfind('.'));
    outFile << "set terminal png" << std::endl;
    outFile << "set output \"" << pngFileName << ".png\"" << std::endl;
    outFile << "set style line 1 lc rgb 'black' ps 2 pt 7" << std::endl;
    outFile << "unset key" << std::endl;
    outFile << "set grid" << std::endl;
    outFile << "plot \"" << posFilename << "\" using 3:4 with points ls 1";
    outFile.close();
}

/**
 * \brief Print vehicle UEs initial position to a file
 * \param filename Name of the file in which to write the positions
 */
void
PrintUeInitPosToFile(std::string filename)
{
    std::ofstream outFile;
    outFile.open(filename.c_str(), std::ios_base::out | std::ios_base::trunc);
    if (!outFile.is_open())
    {
        NS_LOG_ERROR("Can't open file " << filename);
        return;
    }
    for (NodeList::Iterator it = NodeList::Begin(); it != NodeList::End(); ++it)
    {
        Ptr<Node> node = *it;
        int nDevs = node->GetNDevices();
        for (int j = 0; j < nDevs; j++)
        {
            Ptr<NrUeNetDevice> uedev = node->GetDevice(j)->GetObject<NrUeNetDevice>();
            if (uedev)
            {
                Vector pos = node->GetObject<MobilityModel>()->GetPosition();
                outFile << node->GetId() << " " << uedev->GetImsi() << " " << pos.x << " " << pos.y
                        << std::endl;
            }
        }
    }

    WriteInitPosGnuScript(filename);
}

/**
 * \brief Record mobility of the vehicle UEs every second
 * \param FirstWrite If this flag is true, write from scratch, otherwise, append to the file
 * \param fileName Name of the file in which to write the positions
 */
void
RecordMobility(bool FirstWrite, std::string fileName)
{
    std::ofstream outFile;
    if (FirstWrite)
    {
        outFile.open(fileName.c_str(), std::ios_base::out);
        FirstWrite = false;
    }
    else
    {
        outFile.open(fileName.c_str(), std::ios_base::app);
        outFile << std::endl;
        outFile << std::endl;
    }

    for (NodeList::Iterator it = NodeList::Begin(); it != NodeList::End(); ++it)
    {
        Ptr<Node> node = *it;
        int nDevs = node->GetNDevices();
        for (int j = 0; j < nDevs; j++)
        {
            Ptr<NrUeNetDevice> uedev = node->GetDevice(j)->GetObject<NrUeNetDevice>();
            if (uedev)
            {
                Vector pos = node->GetObject<MobilityModel>()->GetPosition();
                outFile << Simulator::Now().GetSeconds() << " " << node->GetId() << " "
                        << uedev->GetImsi() << " " << pos.x << " " << pos.y << " " << pos.z << std::endl;
            }
        }
    }

    Simulator::Schedule(Seconds(1), &RecordMobility, FirstWrite, fileName);
}

/**
 * \brief Write a gnuplot script to generate gif of the vehicle UEs mobility
 * \param MobilityFileName The name of the file, which this script needs to read to plot positions
 * \param simTime The simulation time
 * \param CAVsSpeed The CAVsSpeed of the vehicles
 * \param firstUeNode The node pointer to the first UE in the simulation
 * \param lastUeNode The node pointer to the last UE in the simulation
 */
void
WriteGifGnuScript(std::string MobilityFileName,
                  Time simTime,
                  double CAVsSpeed,
                  Ptr<Node> firstUeNode,
                  Ptr<Node> lastUeNode)
{
    std::ofstream outFile;
    std::string fileName = "gif-script-" + MobilityFileName;
    outFile.open(fileName.c_str(), std::ios_base::out | std::ios_base::trunc);
    if (!outFile.is_open())
    {
        NS_LOG_ERROR("Can't open file " << fileName);
        return;
    }
    outFile << "set term gif animate delay 100" << std::endl;
    std::string gifFileName;
    gifFileName = MobilityFileName.substr(0, MobilityFileName.rfind('.'));
    outFile << "set output \"" << gifFileName << ".gif"
            << "\"" << std::endl;
    outFile << "unset key" << std::endl;
    outFile << "set grid" << std::endl;

    Vector firstNodePos = firstUeNode->GetObject<MobilityModel>()->GetPosition();
    Vector LastNodePos = lastUeNode->GetObject<MobilityModel>()->GetPosition();
    double xRangeLower = firstNodePos.x - 10.0;
    double xRangeUpper = simTime.GetSeconds() * CAVsSpeed + LastNodePos.x;
    double yRangeLower = firstNodePos.y - 10.0;
    double yRangeUpper = LastNodePos.y + 10.0;
    outFile << "set xrange [" << xRangeLower << ":" << xRangeUpper << "]" << std::endl;
    outFile << "set yrange [" << yRangeLower << ":" << yRangeUpper << "]" << std::endl;
    outFile << "do for [i=0:" << simTime.GetSeconds() - 1 << "] {plot \"" << MobilityFileName
            << "\" index i using 4:5}" << std::endl;
}

/**
 * \brief Get sidelink bitmap from string
 * \param slBitMapString The sidelink bitmap string
 * \param slBitMapVector The vector passed to store the converted sidelink bitmap
 */
void
GetSlBitmapFromString(std::string slBitMapString, std::vector<std::bitset<1>>& slBitMapVector)
{
    static std::unordered_map<std::string, uint8_t> lookupTable = {
        {"0", 0},
        {"1", 1},
    };

    std::stringstream ss(slBitMapString);
    std::string token;
    std::vector<std::string> extracted;

    while (std::getline(ss, token, '|'))
    {
        extracted.push_back(token);
    }

    for (const auto& v : extracted)
    {
        if (lookupTable.find(v) == lookupTable.end())
        {
            NS_FATAL_ERROR("Bit type " << v << " not valid. Valid values are: 0 and 1");
        }
        slBitMapVector.emplace_back(lookupTable[v] & 0x01);
    }
}

/**
 * \brief Save position of the UE as per its IP address
 * \param v2xKpi pointer to the V2xKpi API storing the IP of an UE and its position.
 */
void
SavePositionPerIP(V2xKpi* v2xKpi)
{
    for (NodeList::Iterator it = NodeList::Begin(); it != NodeList::End(); ++it)
    {
        Ptr<Node> node = *it;
        int nDevs = node->GetNDevices();
        for (int j = 0; j < nDevs; j++)
        {
            Ptr<NrUeNetDevice> uedev = node->GetDevice(j)->GetObject<NrUeNetDevice>();
            if (uedev)
            {
                Ptr<Ipv4L3Protocol> ipv4Protocol = node->GetObject<Ipv4L3Protocol>();
                Ipv4InterfaceAddress addresses = ipv4Protocol->GetAddress(1, 0);
                std::ostringstream ueIpv4Addr;
                ueIpv4Addr.str("");
                ueIpv4Addr << addresses.GetLocal();
                Vector pos = node->GetObject<MobilityModel>()->GetPosition();
                v2xKpi->FillPosPerIpMap(ueIpv4Addr.str(), pos);
            }
        }
    }
}
int
main(int argc, char* argv[])
{
    /*
     * Variables that represent the parameters we will accept as input by the
     * command line. Each of them is initialized with a default value.
     */
    double gNBheight = 25.0; // m
    uint16_t numCAVsPerSector = 7;
    uint16_t numSector = 6;
    int numugv = 12;
    int numoperators = 24;
    double CAVsSpeed = 5.0 / 3.6;        // m/s, default 5 km/h
    bool enableOneRxPerSector = true;
    bool logging = true;
    bool harqEnabled = true;
    Time delayBudget = Seconds(0); // Use T2 configuration

    // Traffic parameters 
    bool useIPv6 = false; // default IPV4
    uint32_t udpPacketSize4k = 1500;
    uint32_t udpPacketSizeControl = 200;

    double dataRatecavrsu = 120256.0; // 120.256 Mbps per second
    double dataRatersucav = 200.0; // 200 kbps
    double dataRatecavcav = 200.0; // 200 kbps
    double dataRateddcdo = 256.0; // 200 kbps
    double dataRateddcugv = dataRatersucav; // 200 kbps
    double dataRatedoddc = 300.0 + 256.0; // 300 kbps + 256 kbps
    double dataRatedodo = 128.0; // 128 kbps
    double dataRateugvddc = 32500.0 + 256.0; // 32.5 Mbps + 256 kbps

    // Simulation parameters.
    Time simTime = Seconds(1.0);
    // Sidelink bearers activation time
    Time slBearersActivationTime = Seconds(1.0);

    // NR parameters. We will take the input from the command line, and then we
    // will pass them inside the NR module.
    double centralFrequencyBandSl = 26.5e9; // band n257  TDD //Here band is analogous to channel
    uint16_t bandwidthBandSl = 2000;         // Multiple of 100 KHz; 2000=200 MHz
    double centralFrequencyBandFR1 = 3.410e9; 
    uint16_t bandwidthBandFR1 = 400;         // Multiple of 100 KHz , 400=40 MHz
    std::string errorModel = "ns3::NrEesmIrT1";
    std::string scheduler = "ns3::NrMacSchedulerTdmaRR";
    std::string beamformingMethod = "ns3::DirectPathBeamforming";
    uint16_t numerology = 1;

    double txPower = 29.0;                    // dBm
    std::string tddPattern = "UL|UL|UL|UL|UL|UL|UL|UL|UL|UL|";
    std::string tddgnbPattern = "DL|DL|DL|DL|DL|UL|UL|UL|UL|UL|";
    std::string slBitMap = "1|1|1|1|1|1|1|1|1|1|";
    uint16_t numerologyBwpSl = 3;
    uint16_t slSensingWindow = 100; // T0 in ms
    uint16_t slSelectionWindow = 1; // T2min
    uint16_t slSubchannelSize = 10;
    uint16_t slMaxNumPerReserve = 1;
    double slProbResourceKeep = 0.0;
    uint16_t slMaxTxTransNumPssch = 3;
    uint16_t reservationPeriod = 10; // in ms
    bool enableSensing = true;
    uint16_t t1 = 0;
    uint16_t t2 = 15;
    int slThresPsschRsrp = -128;
    bool enableChannelRandomness = true;
    uint16_t channelUpdatePeriod = 500; // ms
    uint8_t mcs = 28;

    // flags to generate gnuplot plotting scripts
    bool generateInitialPosGnuScript = false;
    bool generateGifGnuScript = false;

    // Where we will store the output files.
    std::string outputDir = "./";

    /*
     * From here, we instruct the ns3::CommandLine class of all the input parameters
     * that we may accept as input, as well as their description, and the storage
     * variable.
     */
    CommandLine cmd(__FILE__);
    cmd.AddValue("gNBheight", "The height of the gNB in meters", gNBheight);
    cmd.AddValue("logging", "Enable logging", logging);
    cmd.AddValue("numCAVsPerSector", "Number of vehicles per numSector", numCAVsPerSector);
    cmd.AddValue("numSector", "Total Number of numSector", numSector);
    cmd.AddValue("CAVsSpeed", "Speed of the CAVs in m/sec", CAVsSpeed);
    cmd.AddValue("enableOneRxPerSector",
                 "Per lane make one vehicle a transmitter. This option only works"
                 "with odd number of UEs per lane, which makes the middle vehicle"
                 "in each lane a transmitter",
                 enableOneRxPerSector);
    cmd.AddValue("useIPv6", "Use IPv6 instead of IPv4", useIPv6);
    cmd.AddValue("packetSizeBe",
                 "packet size in bytes to be used by 4k video survelience traffic",
                 udpPacketSize4k);
    cmd.AddValue("packetSizeControl",
                 "packet size in bytes to be used by control traffic",
                 udpPacketSizeControl);
    cmd.AddValue("dataRatecavrsu",
                 "The data rate in kilobits per second for cav->rsu traffic",
                 dataRatecavrsu);
    cmd.AddValue("dataRatersucav",
                 "The data rate in kilobits per second for rsu->cav traffic",
                 dataRatersucav);
    cmd.AddValue("dataRatecavcav",
                 "The data rate in kilobits per second for cav->cav traffic",
                 dataRatecavcav);
    cmd.AddValue("simTime", "Simulation time in seconds", simTime);
    cmd.AddValue("slBearerActivationTime",
                 "Sidelik bearer activation time in seconds",
                 slBearersActivationTime);
    cmd.AddValue("centralFrequencyBandSl",
                 "The central frequency to be used for Sidelink band/channel",
                 centralFrequencyBandSl);
    cmd.AddValue("bandwidthBandSl",
                 "The system bandwidth to be used for Sidelink",
                 bandwidthBandSl);
    cmd.AddValue("txPower", "total tx power in dBm", txPower);
    cmd.AddValue("tddPattern", "The TDD pattern string", tddPattern);
    cmd.AddValue("slBitMap", "The Sidelink bitmap string", slBitMap);
    cmd.AddValue("numerologyBwpSl",
                 "The numerology to be used in Sidelink bandwidth part",
                 numerologyBwpSl);
    cmd.AddValue("slSensingWindow", "The Sidelink sensing window length in ms", slSensingWindow);
    cmd.AddValue("slSelectionWindow",
                 "The parameter which decides the minimum Sidelink selection "
                 "window length in physical slots. T2min = slSelectionWindow * 2^numerology",
                 slSelectionWindow);
    cmd.AddValue("slSubchannelSize", "The Sidelink subchannel size in RBs", slSubchannelSize);
    cmd.AddValue("slMaxNumPerReserve",
                 "The parameter which indicates the maximum number of reserved "
                 "PSCCH/PSSCH resources that can be indicated by an SCI.",
                 slMaxNumPerReserve);
    cmd.AddValue("slProbResourceKeep",
                 "The parameter which indicates the probability with which the "
                 "UE keeps the current resource when the resource reselection"
                 "counter reaches zero.",
                 slProbResourceKeep);
    cmd.AddValue("slMaxTxTransNumPssch",
                 "The parameter which indicates the maximum transmission number "
                 "(including new transmission and retransmission) for PSSCH.",
                 slMaxTxTransNumPssch);
    cmd.AddValue("enableSensing",
                 "If true, it enables the sensing based resource selection for "
                 "SL, otherwise, no sensing is applied",
                 enableSensing);
    cmd.AddValue("t1",
                 "The start of the selection window in physical slots, "
                 "accounting for physical layer processing delay",
                 t1);
    cmd.AddValue("t2", "The end of the selection window in physical slots", t2);
    cmd.AddValue("slThresPsschRsrp",
                 "A threshold in dBm used for sensing based UE autonomous resource selection",
                 slThresPsschRsrp);
    cmd.AddValue("enableChannelRandomness",
                 "Enable shadowing and channel updates",
                 enableChannelRandomness);
    cmd.AddValue("channelUpdatePeriod", "The channel update period in ms", channelUpdatePeriod);
    cmd.AddValue("mcs", "The MCS to used for sidelink", mcs);
    cmd.AddValue("outputDir", "directory where to store simulation results", outputDir);
    cmd.AddValue("generateInitialPosGnuScript",
                 "generate gnuplot script to plot initial positions of the UEs",
                 generateInitialPosGnuScript);
    cmd.AddValue("generateGifGnuScript",
                 "generate gnuplot script to generate GIF to show UEs mobility",
                 generateGifGnuScript);

    // Parse the command line
    cmd.Parse(argc, argv);

    if (logging)
    {
        LogLevel logLevel =
            (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL);
        //LogComponentEnable("OnOffApplication", logLevel);
        //LogComponentEnable("PacketSink", logLevel);
        //LogComponentEnable("LtePdcp", logLevel);
        //LogComponentEnable("NrSlHelper", logLevel);
        //LogComponentEnable("NrSlUeRrc", logLevel);
        //LogComponentEnable("NrUeMac", logLevel);
        LogComponentEnable("NrUePhy", logLevel);
        //LogComponentEnable("NrSpectrumPhy", logLevel);
        //LogComponentEnable("NrGnbMac", logLevel);
        //LogComponentEnable("NrGnbPhy", logLevel);   
    }

    /*
     * Default values for the simulation.
     */
    Config::SetDefault("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(999999999));
    
    
    /*
     * Create a NodeContainer for all the UEs
     */
    NodeContainer allSlUesContainer;
    NodeContainer allFR1UEContainer;

    /*
     * Assign mobility to the UEs.
     *  1. Set mobility model type.
     *  2. Assign position to the UEs
     *  3. Install mobility model
     */
    allSlUesContainer = InstallSLMobility(numSector, numCAVsPerSector, CAVsSpeed);
    allFR1UEContainer = InstallFR1Mobility(numugv, CAVsSpeed , numoperators, CAVsSpeed);

    NodeContainer gnbContainer;
    gnbContainer.Create(1);
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, gNBheight));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(gnbContainer.Get(0));
    
    Packet::EnableChecking();
    Packet::EnablePrinting();
    /*
     * Setup the NR module. We create the various helpers needed for the
     * NR simulation:
     * - EpcHelper, which will setup the core network
     * - NrHelper, which takes care of creating and connecting the various
     * part of the NR stack
     */
    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper>();
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
    nrHelper->SetEpcHelper(epcHelper);
    Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
    nrHelper->SetBeamformingHelper(idealBeamformingHelper);
    
    /*
     * Spectrum division. We create one operational band, containing
     * one component carrier, and a single bandwidth part
     * centered at the frequency specified by the input parameters.
     * We will use the StreetCanyon channel modeling.
     */

    BandwidthPartInfoPtrVector allBwps;
    BandwidthPartInfoPtrVector allBwpsFR1;
    CcBwpCreator ccBwpCreator;
    const uint8_t numCcPerBand = 1;

    /* Create the configuration for the CcBwpHelper. SimpleOperationBandConf
     * creates a single BWP per CC
     */
    
    CcBwpCreator::SimpleOperationBandConf bandConfFR1(centralFrequencyBandFR1,
                                                     bandwidthBandFR1,
                                                     numCcPerBand,
                                                     BandwidthPartInfo::UMi_StreetCanyon);

    // By using the configuration created, it is time to make the operation bands
    OperationBandInfo bandFR1 = ccBwpCreator.CreateOperationBandContiguousCc(bandConfFR1);

    CcBwpCreator::SimpleOperationBandConf bandConfSl(centralFrequencyBandSl,
                                                     bandwidthBandSl,
                                                     numCcPerBand,
                                                     BandwidthPartInfo::V2V_Highway);

    // By using the configuration created, it is time to make the operation bands
    OperationBandInfo bandSl = ccBwpCreator.CreateOperationBandContiguousCc(bandConfSl);

    

    /*
     * The configured spectrum division is:
     * ------------Band1-----------------Band2--------------
     * ------------CC1-------------------Band2--------------
     * ------------BwpSl-----------------BandFR1------------
     */
    if (enableChannelRandomness)
    {
        Config::SetDefault("ns3::ThreeGppChannelModel::UpdatePeriod",
                           TimeValue(MilliSeconds(channelUpdatePeriod)));
        nrHelper->SetChannelConditionModelAttribute("UpdatePeriod",
                                                    TimeValue(MilliSeconds(channelUpdatePeriod)));
        nrHelper->SetPathlossAttribute("ShadowingEnabled", BooleanValue(true));
    }
    else
    {
        Config::SetDefault("ns3::ThreeGppChannelModel::UpdatePeriod", TimeValue(MilliSeconds(0)));
        nrHelper->SetChannelConditionModelAttribute("UpdatePeriod", TimeValue(MilliSeconds(100)));
        nrHelper->SetPathlossAttribute("ShadowingEnabled", BooleanValue(false));
    }

    /*
     * Initialize channel and pathloss, plus other things inside bandSl. If needed,
     * the band configuration can be done manually, but we leave it for more
     * sophisticated examples. For the moment, this method will take care
     * of all the spectrum initialization needs.
     */ 
    nrHelper->InitializeOperationBand(&bandFR1);
    allBwpsFR1 = CcBwpCreator::GetAllBwps({bandFR1});
    
    nrHelper->InitializeOperationBand(&bandSl);
    allBwps = CcBwpCreator::GetAllBwps({bandSl});

    /*
     * Antennas for all the UEs
     * Using quasi-omnidirectional transmission and reception, which is the default
     * configuration of the beams.
     *
     * Following attribute would be common for all the UEs
     */
    nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(8));
    nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(8));
    Ptr<IsotropicAntennaModel> ueIsotropicAntenna = CreateObject<IsotropicAntennaModel>();
    ueIsotropicAntenna->SetAttribute("Gain", DoubleValue(11.0));
    nrHelper->SetUeAntennaAttribute("AntennaElement", PointerValue(ueIsotropicAntenna));
    // Antennas for all the gNbs
    nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(4));
    nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(8));
    nrHelper->SetGnbAntennaAttribute("AntennaElement",
    PointerValue(CreateObject<IsotropicAntennaModel>()));

    nrHelper->SetDlErrorModel(errorModel);
    nrHelper->SetUlErrorModel(errorModel);
    nrHelper->SetGnbDlAmcAttribute("AmcModel", EnumValue(NrAmc::ErrorModel));
    nrHelper->SetGnbUlAmcAttribute("AmcModel", EnumValue(NrAmc::ErrorModel));
    nrHelper->SetSchedulerTypeId(TypeId::LookupByName(scheduler));
    idealBeamformingHelper->SetAttribute("BeamformingMethod",
                                         TypeIdValue(TypeId::LookupByName(beamformingMethod)));
                                          // Core latency
    epcHelper->SetAttribute("S1uLinkDelay", TimeValue(MilliSeconds(0)));
    
    nrHelper->SetUePhyAttribute("TxPower", DoubleValue(txPower));
    nrHelper->SetGnbPhyAttribute("TxPower", DoubleValue(txPower));
    nrHelper->SetGnbPhyAttribute("Numerology", UintegerValue(numerology));

    nrHelper->SetUeMacTypeId(NrSlUeMac::GetTypeId());
    nrHelper->SetUeMacAttribute("EnableSensing", BooleanValue(enableSensing));
    nrHelper->SetUeMacAttribute("T1", UintegerValue(static_cast<uint8_t>(t1)));
    nrHelper->SetUeMacAttribute("T2", UintegerValue(t2));
    nrHelper->SetUeMacAttribute("ActivePoolId", UintegerValue(0));
    nrHelper->SetUeMacAttribute("SlThresPsschRsrp", IntegerValue(slThresPsschRsrp));
    
    uint8_t bwpIdForGbrMcptt = 0;

    nrHelper->SetBwpManagerTypeId(TypeId::LookupByName("ns3::NrSlBwpManagerUe"));
    nrHelper->SetUeBwpManagerAlgorithmAttribute("GBR_MC_PUSH_TO_TALK",
                                                UintegerValue(bwpIdForGbrMcptt));

    // gNb routing between bearer type and bandwidth part
    nrHelper->SetGnbBwpManagerAlgorithmAttribute("NGBR_LOW_LAT_EMBB", UintegerValue(0));
    
    std::set<uint8_t> bwpIdContainer;
    bwpIdContainer.insert(bwpIdForGbrMcptt);
    
    NetDeviceContainer allSlUesNetDeviceContainer =
        nrHelper->InstallUeDevice(allSlUesContainer, allBwps);
    /*
     * Finally, create the gNB and the UE device.
     */
    NetDeviceContainer gNBNetDev = nrHelper->InstallGnbDevice(gnbContainer, allBwpsFR1);
    NetDeviceContainer ueNetDev = nrHelper->InstallUeDevice(allFR1UEContainer, allBwpsFR1);
    
    nrHelper->GetGnbPhy(gNBNetDev.Get(0), 0)
        ->SetAttribute("Pattern", StringValue(tddgnbPattern));
   
    /*
     * We have configured the attributes we needed. Now, install and get the pointers
     * to the NetDevices, which contains all the NR stack:
     */
    
    int64_t randomStream = 1;
    randomStream += nrHelper->AssignStreams(gNBNetDev, randomStream);
    randomStream += nrHelper->AssignStreams(ueNetDev, randomStream);

    // When all the configuration is done, explicitly call UpdateConfig ()
    for (auto it = allSlUesNetDeviceContainer.Begin(); it != allSlUesNetDeviceContainer.End(); ++it)
    {
        DynamicCast<NrUeNetDevice>(*it)->UpdateConfig();
    }
    for(auto it = ueNetDev.Begin(); it != ueNetDev.End(); ++it)
    {
        DynamicCast<NrUeNetDevice>(*it)->UpdateConfig();
    }
    for(auto it = gNBNetDev.Begin(); it != gNBNetDev.End(); ++it)
    {
        DynamicCast<NrGnbNetDevice>(*it)->UpdateConfig();
    }

    /*
     * Configure Sidelink. We create the following helpers needed for the
     * NR Sidelink, i.e., V2X simulation:
     * - NrSlHelper, which will configure the UEs protocol stack to be ready to
     *   perform Sidelink related procedures.
     * - EpcHelper, which takes care of triggering the call to EpcUeNas class
     *   to establish the NR Sidelink bearer(s). We note that, at this stage
     *   just communicate the pointer of already instantiated EpcHelper object,
     *   which is the same pointer communicated to the NrHelper above.
     */
    Ptr<NrSlHelper> nrSlHelper = CreateObject<NrSlHelper>();
    // Put the pointers inside NrSlHelper
    nrSlHelper->SetEpcHelper(epcHelper);

    /*
     * Set the SL error model and AMC
     * Error model type: ns3::NrEesmCcT1, ns3::NrEesmCcT2, ns3::NrEesmIrT1,
     *                   ns3::NrEesmIrT2, ns3::NrLteMiErrorModel
     * AMC type: NrAmc::ShannonModel or NrAmc::ErrorModel
     */
    nrSlHelper->SetSlErrorModel(errorModel);
    nrSlHelper->SetUeSlAmcAttribute("AmcModel", EnumValue(NrAmc::ErrorModel));

    /*
     * Set the SL scheduler attributes
     * In this i use NrSlUeMacSchedulerSimple scheduler, which uses
     * a fixed MCS value
     */
    nrSlHelper->SetNrSlSchedulerTypeId(NrSlUeMacSchedulerFixedMcs::GetTypeId());
    nrSlHelper->SetUeSlSchedulerAttribute("Mcs", UintegerValue(mcs));

    /*
     * Very important method to configure UE protocol stack, i.e., it would
     * configure all the SAPs among the layers, setup callbacks, configure
     * error model, configure AMC, and configure ChunkProcessor in Interference
     * API.
     */
    nrSlHelper->PrepareUeForSidelink(allSlUesNetDeviceContainer, bwpIdContainer);

    /*
     * Start preparing for all the sub Structs/RRC Information Element (IEs)
     * of LteRrcSap::SidelinkPreconfigNr. This is the main structure, which would
     * hold all the pre-configuration related to Sidelink.
     */

    // SlResourcePoolNr IE
    LteRrcSap::SlResourcePoolNr slResourcePoolNr;
    // get it from pool factory
    Ptr<NrSlCommResourcePoolFactory> ptrFactory = Create<NrSlCommResourcePoolFactory>();
    /*
     * Above pool factory is created to help the users of the simulator to create
     * a pool with valid default configuration. Please have a look at the
     * constructor of NrSlCommResourcePoolFactory class.
     */
    std::vector<std::bitset<1>> slBitMapVector;
    GetSlBitmapFromString(slBitMap, slBitMapVector);
    NS_ABORT_MSG_IF(slBitMapVector.empty(), "GetSlBitmapFromString failed to generate SL bitmap");
    ptrFactory->SetSlTimeResources(slBitMapVector);
    ptrFactory->SetSlSensingWindow(slSensingWindow); // T0 in ms
    ptrFactory->SetSlSelectionWindow(slSelectionWindow);
    ptrFactory->SetSlFreqResourcePscch(10); // PSCCH RBs
    ptrFactory->SetSlSubchannelSize(slSubchannelSize);
    ptrFactory->SetSlMaxNumPerReserve(slMaxNumPerReserve);
    std::list<uint16_t> resourceReservePeriodList = {0, reservationPeriod}; // in ms
    ptrFactory->SetSlResourceReservePeriodList(resourceReservePeriodList);
    // Once parameters are configured, we can create the pool
    LteRrcSap::SlResourcePoolNr pool = ptrFactory->CreatePool();
    slResourcePoolNr = pool;

    // Configure the SlResourcePoolConfigNr IE, which hold a pool and its id
    LteRrcSap::SlResourcePoolConfigNr slresoPoolConfigNr;
    slresoPoolConfigNr.haveSlResourcePoolConfigNr = true;
    // Pool id, ranges from 0 to 15
    uint16_t poolId = 0;
    LteRrcSap::SlResourcePoolIdNr slResourcePoolIdNr;
    slResourcePoolIdNr.id = poolId;
    slresoPoolConfigNr.slResourcePoolId = slResourcePoolIdNr;
    slresoPoolConfigNr.slResourcePool = slResourcePoolNr;

    // Configure the SlBwpPoolConfigCommonNr IE, which hold an array of pools
    LteRrcSap::SlBwpPoolConfigCommonNr slBwpPoolConfigCommonNr;
    // Array for pools, we insert the pool in the array as per its poolId
    slBwpPoolConfigCommonNr.slTxPoolSelectedNormal[slResourcePoolIdNr.id] = slresoPoolConfigNr;

    // Configure the BWP IE
    LteRrcSap::Bwp bwp;
    bwp.numerology = numerologyBwpSl;
    bwp.symbolsPerSlots = 14;
    bwp.rbPerRbg = 1;
    bwp.bandwidth = bandwidthBandSl;

    // Configure the SlBwpGeneric IE
    LteRrcSap::SlBwpGeneric slBwpGeneric;
    slBwpGeneric.bwp = bwp;
    slBwpGeneric.slLengthSymbols = LteRrcSap::GetSlLengthSymbolsEnum(14);
    slBwpGeneric.slStartSymbol = LteRrcSap::GetSlStartSymbolEnum(0);

    // Configure the SlBwpConfigCommonNr IE
    LteRrcSap::SlBwpConfigCommonNr slBwpConfigCommonNr;
    slBwpConfigCommonNr.haveSlBwpGeneric = true;
    slBwpConfigCommonNr.slBwpGeneric = slBwpGeneric;
    slBwpConfigCommonNr.haveSlBwpPoolConfigCommonNr = true;
    slBwpConfigCommonNr.slBwpPoolConfigCommonNr = slBwpPoolConfigCommonNr;

    // Configure the SlFreqConfigCommonNr IE, which hold the array to store
    // the configuration of all Sidelink BWP (s).
    LteRrcSap::SlFreqConfigCommonNr slFreConfigCommonNr;
    // Array for BWPs. Here we will iterate over the BWPs, which
    // we want to use for SL.
    for (const auto& it : bwpIdContainer)
    {
        // it is the BWP id
        slFreConfigCommonNr.slBwpList[it] = slBwpConfigCommonNr;
    }

    // Configure the TddUlDlConfigCommon IE
    LteRrcSap::TddUlDlConfigCommon tddUlDlConfigCommon;
    tddUlDlConfigCommon.tddPattern = tddPattern;

    // Configure the SlPreconfigGeneralNr IE
    LteRrcSap::SlPreconfigGeneralNr slPreconfigGeneralNr;
    slPreconfigGeneralNr.slTddConfig = tddUlDlConfigCommon;

    // Configure the SlUeSelectedConfig IE
    LteRrcSap::SlUeSelectedConfig slUeSelectedPreConfig;
    NS_ABORT_MSG_UNLESS(slProbResourceKeep <= 1.0,
                        "slProbResourceKeep value must be between 0 and 1");
    slUeSelectedPreConfig.slProbResourceKeep = slProbResourceKeep;
    // Configure the SlPsschTxParameters IE
    LteRrcSap::SlPsschTxParameters psschParams;
    psschParams.slMaxTxTransNumPssch = static_cast<uint8_t>(slMaxTxTransNumPssch);
    // Configure the SlPsschTxConfigList IE
    LteRrcSap::SlPsschTxConfigList pscchTxConfigList;
    pscchTxConfigList.slPsschTxParameters[0] = psschParams;
    slUeSelectedPreConfig.slPsschTxConfigList = pscchTxConfigList;

    /*
     * Finally, configure the SidelinkPreconfigNr. This is the main structure
     * that needs to be communicated to NrSlUeRrc class
     */
    LteRrcSap::SidelinkPreconfigNr slPreConfigNr;
    slPreConfigNr.slPreconfigGeneral = slPreconfigGeneralNr;
    slPreConfigNr.slUeSelectedPreConfig = slUeSelectedPreConfig;
    slPreConfigNr.slPreconfigFreqInfoList[0] = slFreConfigCommonNr;

    // Communicate the above pre-configuration to the NrSlHelper
    nrSlHelper->InstallNrSlPreConfiguration(allSlUesNetDeviceContainer, slPreConfigNr);

    /****************************** End SL Configuration ***********************/

    /*
     * Fix the random streams
     */
    int64_t stream = 1;
    stream += nrHelper->AssignStreams(allSlUesNetDeviceContainer, stream);
    stream += nrSlHelper->AssignStreams(allSlUesNetDeviceContainer, stream);
    
    /*
    
     * if enableOneRxPerSector is true:
     *
     * All the UEs can transmit and receive
     */
    NodeContainer CavUEs;
    NodeContainer RsuUes;
    NetDeviceContainer CavUEsNetDevice;
    NetDeviceContainer RsuUesNetDevice;

    NodeContainer DoUEs;
    NodeContainer UgvUes;
    NodeContainer DDCUe;
    NetDeviceContainer DoUEsNetDevice;
    NetDeviceContainer UgvUesNetDevice;
    NetDeviceContainer DDCUeNetDevice;

    if (enableOneRxPerSector)
    {
        for (uint16_t i = 0; i < numSector * numCAVsPerSector; i++)
        {
            // for each Sector one rsu and rest cav
            if (i % numCAVsPerSector == 0) {
                RsuUes.Add(allSlUesContainer.Get(i));
                RsuUesNetDevice.Add(allSlUesContainer.Get(i)->GetDevice(0));
            }else{  
                CavUEs.Add(allSlUesContainer.Get(i));
                CavUEsNetDevice.Add(allSlUesContainer.Get(i)->GetDevice(0));  
            }
        }
        for(uint16_t i = 0; i < numugv; i++)
        {  
            UgvUes.Add(allFR1UEContainer.Get(i));
            UgvUesNetDevice.Add(allFR1UEContainer.Get(i)->GetDevice(0));
        }
        for(uint16_t i = numugv; i < numugv + numoperators; i++)
        {
            DoUEs.Add(allFR1UEContainer.Get(i));
            DoUEsNetDevice.Add(allFR1UEContainer.Get(i)->GetDevice(0));
        }
        for(uint16_t i = numugv + numoperators; i < numugv + numoperators + 1; i++)
        {
            DDCUe.Add(allFR1UEContainer.Get(i));
            DDCUeNetDevice.Add(allFR1UEContainer.Get(i)->GetDevice(0));
        }
    }
    else
    {
        CavUEs.Add(allSlUesContainer);
        RsuUes.Add(allSlUesContainer);
        CavUEsNetDevice.Add(allSlUesNetDeviceContainer);
        RsuUesNetDevice.Add(allSlUesNetDeviceContainer);
    }

    /*
     * Configure the IP stack, and activate NR Sidelink bearer (s) as per the
     * configured time.
     *
     * This example supports IPV4 and IPV6
     */

    InternetStackHelper internet;
    internet.Install(allSlUesContainer);
    stream += internet.AssignStreams(allSlUesContainer, stream);
    
    internet.Install(allFR1UEContainer);
    randomStream += internet.AssignStreams(allFR1UEContainer, randomStream);

    // get SGW/PGW and create a single RemoteHost
    Ptr<Node> pgw = epcHelper->GetPgwNode();
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    internet.Install(remoteHostContainer);

    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(2500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.000)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    uint32_t dstL2Groupcast = 254;
    Ipv4Address groupAddress4("225.0.0.0"); // use multicast address as destination
    Ipv6Address groupAddress6("ff0e::1");   // use multicast address as destination
    //sl adresses
    Address remoteAddress;
    Address localAddress;
    Address remoteAddress2;
    Address localAddress2;
    Address remoteAddress3;
    Address localAddress3;
    //fr1 adresses
    Address remoteAddress4;
    Address localAddress4;
    Address remoteAddress5;
    Address localAddress5;

    Address remoteAddress6;
    Address localAddress6;
    Address remoteAddress7;
    Address localAddress7;
    Address remoteAddress8;
    Address localAddress8;
    //sl ports
    uint16_t port = 8000;
    uint16_t port2 = 8001;
    uint16_t port3 = 8002;
    //fr1 ports
    uint16_t port4 = 1234;
    uint16_t port5 = 1235;

    uint16_t port6 = 1236;
    uint16_t port7 = 1237;
    uint16_t port8 = 1238;
    
    Ptr<LteSlTft> tft;

    SidelinkInfo slInfo;
    slInfo.m_castType = SidelinkInfo::CastType::Groupcast;
    slInfo.m_dstL2Id = dstL2Groupcast;
    slInfo.m_rri = MilliSeconds(reservationPeriod);
    slInfo.m_dynamic = false;
    slInfo.m_pdb = delayBudget;
    slInfo.m_harqEnabled = harqEnabled;

    SidelinkInfo slInfo2;
    slInfo2.m_castType = SidelinkInfo::CastType::Groupcast;
    slInfo2.m_dstL2Id = dstL2Groupcast;
    slInfo2.m_rri = MilliSeconds(reservationPeriod);
    slInfo2.m_dynamic = false;
    slInfo2.m_pdb = delayBudget;
    slInfo2.m_harqEnabled = harqEnabled;

    SidelinkInfo slInfo3;
    slInfo3.m_castType = SidelinkInfo::CastType::Groupcast;
    slInfo3.m_dstL2Id = dstL2Groupcast;
    slInfo3.m_rri = MilliSeconds(reservationPeriod);
    slInfo3.m_dynamic = false;
    slInfo3.m_pdb = delayBudget;
    slInfo3.m_harqEnabled = harqEnabled;

    if (!useIPv6)
    {
        Ipv4InterfaceContainer ueIpIface;
        ueIpIface = epcHelper->AssignUeIpv4Address(allSlUesNetDeviceContainer);
        ueIpIface = epcHelper->AssignUeIpv4Address(ueNetDev);
        
        Ipv4StaticRoutingHelper ipv4RoutingHelper;
        for (uint32_t u = 0; u < allSlUesContainer.GetN(); ++u)
        {
            // Set the default gateway for the UE
            Ptr<Ipv4StaticRouting> ueStaticRouting =
                ipv4RoutingHelper.GetStaticRouting(allSlUesContainer.Get(u)->GetObject<Ipv4>());
            ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
        }
        for(uint32_t u = 0; u < allFR1UEContainer.GetN(); ++u)
        {
            Ptr<Ipv4StaticRouting> ueStaticRouting =
                ipv4RoutingHelper.GetStaticRouting(allFR1UEContainer.Get(u)->GetObject<Ipv4>());
            ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);
        }
        nrHelper->AttachToClosestEnb(DoUEsNetDevice, gNBNetDev);
        nrHelper->AttachToClosestEnb(UgvUesNetDevice, gNBNetDev);
        nrHelper->AttachToClosestEnb(DDCUeNetDevice, gNBNetDev);
        //sl
        remoteAddress = InetSocketAddress(groupAddress4, port);
        localAddress = InetSocketAddress(Ipv4Address::GetAny(), port);
        remoteAddress2 = InetSocketAddress(groupAddress4, port2);
        localAddress2 = InetSocketAddress(Ipv4Address::GetAny(), port2);
        remoteAddress3 = InetSocketAddress(groupAddress4, port3);
        localAddress3 = InetSocketAddress(Ipv4Address::GetAny(), port3);
        //fr1 
        remoteAddress4 = InetSocketAddress(groupAddress4, port4);
        localAddress4 = InetSocketAddress(Ipv4Address::GetAny(), port4);
        remoteAddress5 = InetSocketAddress(groupAddress4, port5);
        localAddress5 = InetSocketAddress(Ipv4Address::GetAny(), port5);

        remoteAddress6 = InetSocketAddress(groupAddress4, port6);
        localAddress6 = InetSocketAddress(Ipv4Address::GetAny(), port6);
        remoteAddress7 = InetSocketAddress(groupAddress4, port7);
        localAddress7 = InetSocketAddress(Ipv4Address::GetAny(), port7);
        remoteAddress8 = InetSocketAddress(groupAddress4, port8);
        localAddress8 = InetSocketAddress(Ipv4Address::GetAny(), port8);

        tft = Create<LteSlTft>(LteSlTft::Direction::TRANSMIT, groupAddress4, port, slInfo);
        // Set Sidelink bearers
        nrSlHelper->ActivateNrSlBearer(slBearersActivationTime, CavUEsNetDevice, tft);

        tft = Create<LteSlTft>(LteSlTft::Direction::RECEIVE, groupAddress4, port, slInfo);
        // Set Sidelink bearers
        nrSlHelper->ActivateNrSlBearer(slBearersActivationTime, RsuUesNetDevice, tft);

        tft = Create<LteSlTft>(LteSlTft::Direction::TRANSMIT, groupAddress4, port2, slInfo2);
        // Set Sidelink bearers
        nrSlHelper->ActivateNrSlBearer(slBearersActivationTime, RsuUesNetDevice, tft);

        tft = Create<LteSlTft>(LteSlTft::Direction::RECEIVE, groupAddress4, port2, slInfo2);
        // Set Sidelink bearers
        nrSlHelper->ActivateNrSlBearer(slBearersActivationTime, CavUEsNetDevice, tft);

        tft = Create<LteSlTft>(LteSlTft::Direction::TRANSMIT, groupAddress4, port3, slInfo3);
        // Set Sidelink bearers
        nrSlHelper->ActivateNrSlBearer(slBearersActivationTime, CavUEsNetDevice, tft);

        tft = Create<LteSlTft>(LteSlTft::Direction::RECEIVE, groupAddress4, port3, slInfo3);
        // Set Sidelink bearers
        nrSlHelper->ActivateNrSlBearer(slBearersActivationTime, CavUEsNetDevice, tft);
    }
    else
    {
        Ipv6InterfaceContainer ueIpIface;
        ueIpIface = epcHelper->AssignUeIpv6Address(allSlUesNetDeviceContainer);

        // set the default gateway for the UE
        Ipv6StaticRoutingHelper ipv6RoutingHelper;
        for (uint32_t u = 0; u < allSlUesContainer.GetN(); ++u)
        {
            Ptr<Node> ueNode = allSlUesContainer.Get(u);
            // Set the default gateway for the UE
            Ptr<Ipv6StaticRouting> ueStaticRouting =
                ipv6RoutingHelper.GetStaticRouting(ueNode->GetObject<Ipv6>());
            ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress6(), 1);
        }
        remoteAddress = Inet6SocketAddress(groupAddress6, port);
        localAddress = Inet6SocketAddress(Ipv6Address::GetAny(), port);

        tft = Create<LteSlTft>(LteSlTft::Direction::TRANSMIT, groupAddress4, slInfo);
        // Set Sidelink bearers for transmitting UEs
        nrSlHelper->ActivateNrSlBearer(slBearersActivationTime, CavUEsNetDevice, tft);

        tft = Create<LteSlTft>(LteSlTft::Direction::RECEIVE, groupAddress4, slInfo);
        nrSlHelper->ActivateNrSlBearer(slBearersActivationTime, RsuUesNetDevice, tft);
    }
   
    /*
     * Configure the applications:
     * Client app: OnOff application configure to generate CBR traffic
     * Server app: PacketSink application.
     */
    // Random variable to randomize a bit start times of the client applications
    // to avoid simulation artifacts of all the TX UEs transmitting at the same time.
    Ptr<UniformRandomVariable> startTimeSeconds = CreateObject<UniformRandomVariable>();
    startTimeSeconds->SetStream(stream);
    startTimeSeconds->SetAttribute("Min", DoubleValue(0));
    startTimeSeconds->SetAttribute("Max", DoubleValue(0.10));

    // Set Application in the UEs
    OnOffHelper sidelinkClient("ns3::UdpSocketFactory", remoteAddress);
    sidelinkClient.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));
    std::string dataRatecavrsuString = std::to_string(dataRatecavrsu) + "kb/s";
    std::cout << "Data rate " << DataRate(dataRatecavrsuString) << std::endl;
    sidelinkClient.SetConstantRate(DataRate(dataRatecavrsuString), udpPacketSize4k);

    OnOffHelper sidelinkClient2("ns3::UdpSocketFactory", remoteAddress2);
    sidelinkClient2.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));
    std::string dataRatersucavString = std::to_string(dataRatersucav) + "kb/s";
    std::cout << "Data rate " << DataRate(dataRatersucavString) << std::endl;
    sidelinkClient2.SetConstantRate(DataRate(dataRatersucavString), udpPacketSizeControl);

    OnOffHelper sidelinkClient3("ns3::UdpSocketFactory", remoteAddress3);
    sidelinkClient3.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));
    std::string dataRatecavcavString = std::to_string(dataRatecavcav) + "kb/s";
    std::cout << "Data rate " << DataRate(dataRatecavcavString) << std::endl;
    sidelinkClient3.SetConstantRate(DataRate(dataRatecavcavString), udpPacketSizeControl);

    OnOffHelper fr1Client1("ns3::UdpSocketFactory", remoteAddress4);
    fr1Client1.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));
    std::string dataRateddcdoString = std::to_string(dataRateddcdo) + "kb/s";
    std::cout << "Data rate " << DataRate(dataRateddcdoString) << std::endl;
    fr1Client1.SetConstantRate(DataRate(dataRateddcdoString), udpPacketSizeControl);
     // The bearer that will carry low latency traffic
    EpsBearer lowLatBearer(EpsBearer::NGBR_LOW_LAT_EMBB);
    // The filter for the low-latency traffic
    Ptr<EpcTft> lowLatTft = Create<EpcTft>();
    EpcTft::PacketFilter dlpfLowLat;
    dlpfLowLat.localPortStart = port4;
    dlpfLowLat.localPortEnd = port4;
    lowLatTft->Add(dlpfLowLat);

    OnOffHelper fr1Client2("ns3::UdpSocketFactory", remoteAddress5);
    fr1Client2.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));
    std::string dataRateddcugvString = std::to_string(dataRateddcugv) + "kb/s";
    std::cout << "Data rate " << DataRate(dataRateddcugvString) << std::endl;
    fr1Client2.SetConstantRate(DataRate(dataRateddcugvString), udpPacketSizeControl);
    // The filter for the low-latency traffic
    Ptr<EpcTft> lowLatTft2 = Create<EpcTft>();
    EpcTft::PacketFilter dlpfLowLat2;
    dlpfLowLat2.localPortStart = port5;
    dlpfLowLat2.localPortEnd = port5;
    lowLatTft2->Add(dlpfLowLat2);

    OnOffHelper fr1Client3("ns3::UdpSocketFactory", remoteAddress6);
    fr1Client3.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));
    std::string dataRatedoddcString = std::to_string(dataRatedoddc) + "kb/s";
    std::cout << "Data rate " << DataRate(dataRatedoddcString) << std::endl;
    fr1Client3.SetConstantRate(DataRate(dataRatedoddcString), udpPacketSize4k);
    // The filter for the low-latency traffic
    Ptr<EpcTft> lowLatTft3 = Create<EpcTft>();
    EpcTft::PacketFilter dlpfLowLat3;
    dlpfLowLat3.localPortStart = port6;
    dlpfLowLat3.localPortEnd = port6;
    lowLatTft3->Add(dlpfLowLat3);

    OnOffHelper fr1Client4("ns3::UdpSocketFactory", remoteAddress7);
    fr1Client4.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));
    std::string dataRatedodoString = std::to_string(dataRatedodo) + "kb/s";
    std::cout << "Data rate " << DataRate(dataRatedodoString) << std::endl;
    fr1Client4.SetConstantRate(DataRate(dataRatedodoString), udpPacketSizeControl);
    // The filter for the low-latency traffic
    Ptr<EpcTft> lowLatTft4 = Create<EpcTft>();
    EpcTft::PacketFilter dlpfLowLat4;
    dlpfLowLat4.localPortStart = port7;
    dlpfLowLat4.localPortEnd = port7;
    lowLatTft4->Add(dlpfLowLat4);

    OnOffHelper fr1Client5("ns3::UdpSocketFactory", remoteAddress8);
    fr1Client5.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));
    std::string dataRateugvddcString = std::to_string(dataRateugvddc) + "kb/s";
    std::cout << "Data rate " << DataRate(dataRateugvddcString) << std::endl;
    fr1Client5.SetConstantRate(DataRate(dataRateugvddcString), udpPacketSize4k);
    // The filter for the low-latency traffic
    Ptr<EpcTft> lowLatTft5 = Create<EpcTft>();
    EpcTft::PacketFilter dlpfLowLat5;
    dlpfLowLat5.localPortStart = port8;
    dlpfLowLat5.localPortEnd = port8;
    lowLatTft5->Add(dlpfLowLat5);
    
    ApplicationContainer clientApps;
    ApplicationContainer clientApps2;
    ApplicationContainer clientApps3;

    ApplicationContainer clientFR1Apps1;
    ApplicationContainer clientFR1Apps2;
    ApplicationContainer clientFR1Apps3;
    ApplicationContainer clientFR1Apps4;
    ApplicationContainer clientFR1Apps5;

    double realAppStart = 0.0;
    double realAppStopTime = 0.0;
    double txAppDuration = 0.0;
    double txAppDurationFin = 0.0;
    
    std::ostringstream path;
    int j=0;
 
    for (uint32_t i = 0; i < CavUEs.GetN(); i++)
    {
        clientApps.Add(sidelinkClient.Install(CavUEs.Get(i)));
        double jitter = startTimeSeconds->GetValue();
        Time appStart = slBearersActivationTime + Seconds(jitter);
        clientApps.Get(i)->SetStartTime(appStart);

        // onoff application will send the first packet at :
        // slBearersActivationTime + random jitter + ((Pkt size in bits) / (Data rate in bits per
        // sec))
        realAppStart = slBearersActivationTime.GetSeconds() + jitter +
                       ((double)udpPacketSize4k * 8.0 / (DataRate(dataRatecavrsuString).GetBitRate()));
        realAppStopTime = realAppStart + simTime.GetSeconds();
        clientApps.Get(i)->SetStopTime(Seconds(realAppStopTime));
        txAppDuration = realAppStopTime - realAppStart;

        // Output app start, stop and duration
        std::cout << "Tx App " << j + 1 << " start time " << std::fixed << std::setprecision(5)
                  << realAppStart << " sec" << std::endl;
        std::cout << "Tx App " << j + 1 << " stop time " << realAppStopTime << " sec" << std::endl;
        std::cout << "Tx App duration " << std::defaultfloat << txAppDuration << " sec"
                  << std::endl;

        if(txAppDuration > txAppDurationFin){
            txAppDurationFin = txAppDuration;
        }

        j++;

        //app3 for cav cav link
        clientApps3.Add(sidelinkClient3.Install(CavUEs.Get(i)));
        double jitter1 = startTimeSeconds->GetValue();
        Time appStart1 = slBearersActivationTime + Seconds(jitter1);
        clientApps3.Get(i)->SetStartTime(appStart1);

        // onoff application will send the first packet at :
        // slBearersActivationTime + random jitter + ((Pkt size in bits) / (Data rate in bits per
        // sec))
        realAppStart = slBearersActivationTime.GetSeconds() + jitter1 +
                       ((double)udpPacketSizeControl * 8.0 / (DataRate(dataRatecavcavString).GetBitRate()));
        realAppStopTime = realAppStart + simTime.GetSeconds();
        clientApps3.Get(i)->SetStopTime(Seconds(realAppStopTime));
        txAppDuration = realAppStopTime - realAppStart;

        // Output app start, stop and duration
        std::cout << "Tx App " << j + 1 << " start time " << std::fixed << std::setprecision(5)
                  << realAppStart << " sec" << std::endl;
        std::cout << "Tx App " << j + 1 << " stop time " << realAppStopTime << " sec" << std::endl;
        std::cout << "Tx App duration " << std::defaultfloat << txAppDuration << " sec"
                  << std::endl;
        
        if(txAppDuration > txAppDurationFin){
            txAppDurationFin = txAppDuration;
        }
        j++;
    }
    //app 2 for rsu cav link
    for (uint32_t i = 0; i < RsuUes.GetN(); i++)
    {
        clientApps2.Add(sidelinkClient2.Install(RsuUes.Get(i)));
        double jitter2 = startTimeSeconds->GetValue();
        Time appStart = slBearersActivationTime + Seconds(jitter2);
        clientApps2.Get(i)->SetStartTime(appStart);

        // onoff application will send the first packet at :
        // slBearersActivationTime + random jitter + ((Pkt size in bits) / (Data rate in bits per
        // sec))
        realAppStart = slBearersActivationTime.GetSeconds() + jitter2 +
                       ((double)udpPacketSizeControl * 8.0 / (DataRate(dataRatecavcavString).GetBitRate()));
        realAppStopTime = realAppStart + simTime.GetSeconds();
        clientApps2.Get(i)->SetStopTime(Seconds(realAppStopTime));
        txAppDuration = realAppStopTime - realAppStart;

        // Output app start, stop and duration
        std::cout << "Tx App " << j + 1 << " start time " << std::fixed << std::setprecision(5)
                  << realAppStart << " sec" << std::endl;
        std::cout << "Tx App " << j + 1 << " stop time " << realAppStopTime << " sec" << std::endl;
        std::cout << "Tx App duration " << std::defaultfloat << txAppDuration << " sec"
                  << std::endl;

        if(txAppDuration > txAppDurationFin){
            txAppDurationFin = txAppDuration;
        }
        j++;
    }
    //fr1 apps ddcdo 
    clientFR1Apps1.Add(fr1Client1.Install(DDCUe.Get(0)));
    nrHelper->ActivateDedicatedEpsBearer(DDCUeNetDevice.Get(0), lowLatBearer, lowLatTft);
    
    double jitter2 = startTimeSeconds->GetValue();
    Time appStart = slBearersActivationTime + Seconds(jitter2);
    clientFR1Apps1.Get(0)->SetStartTime(appStart);
    realAppStart = slBearersActivationTime.GetSeconds() + jitter2 +
                       ((double)udpPacketSizeControl * 8.0 / (DataRate(dataRateddcdoString).GetBitRate()));
    realAppStopTime = realAppStart + simTime.GetSeconds();
    clientFR1Apps1.Get(0)->SetStopTime(Seconds(realAppStopTime));

    //fr1 apps ddcugv
    clientFR1Apps2.Add(fr1Client2.Install(DDCUe.Get(0)));
    nrHelper->ActivateDedicatedEpsBearer(DDCUeNetDevice.Get(0), lowLatBearer, lowLatTft2);

    jitter2 = startTimeSeconds->GetValue();
    appStart = slBearersActivationTime + Seconds(jitter2);
    clientFR1Apps1.Get(0)->SetStartTime(appStart);
    realAppStart = slBearersActivationTime.GetSeconds() + jitter2 +
                       ((double)udpPacketSizeControl * 8.0 / (DataRate(dataRateddcugvString).GetBitRate()));
    realAppStopTime = realAppStart + simTime.GetSeconds();
    clientFR1Apps1.Get(0)->SetStopTime(Seconds(realAppStopTime));
    
    for(uint32_t i = 0; i < DoUEs.GetN(); i++)
    {
        //fr1 apps doddc
        clientFR1Apps3.Add(fr1Client3.Install(DoUEs.Get(i)));
        nrHelper->ActivateDedicatedEpsBearer(DoUEsNetDevice.Get(i), lowLatBearer, lowLatTft3);

        double jitter2 = startTimeSeconds->GetValue();
        Time appStart = slBearersActivationTime + Seconds(jitter2);
        clientFR1Apps3.Get(i)->SetStartTime(appStart);
        realAppStart = slBearersActivationTime.GetSeconds() + jitter2 +
                        ((double)udpPacketSize4k * 8.0 / (DataRate(dataRatedoddcString).GetBitRate()));
        realAppStopTime = realAppStart + simTime.GetSeconds();
        clientFR1Apps3.Get(i)->SetStopTime(Seconds(realAppStopTime));

        //fr1 apps dodo
        clientFR1Apps4.Add(fr1Client4.Install(DoUEs.Get(i)));
        nrHelper->ActivateDedicatedEpsBearer(DoUEsNetDevice.Get(i), lowLatBearer, lowLatTft4);

        jitter2 = startTimeSeconds->GetValue();
        appStart = slBearersActivationTime + Seconds(jitter2);
        clientFR1Apps4.Get(i)->SetStartTime(appStart);
        realAppStart = slBearersActivationTime.GetSeconds() + jitter2 +
                        ((double)udpPacketSizeControl * 8.0 / (DataRate(dataRatedodoString).GetBitRate()));
        realAppStopTime = realAppStart + simTime.GetSeconds();
        clientFR1Apps4.Get(i)->SetStopTime(Seconds(realAppStopTime));

    }
    for(uint32_t i = 0; i < UgvUes.GetN(); i++)
    {
        //fr1 apps ugvddc
        clientFR1Apps5.Add(fr1Client5.Install(UgvUes.Get(i)));
        nrHelper->ActivateDedicatedEpsBearer(UgvUesNetDevice.Get(i), lowLatBearer, lowLatTft4);

        double jitter2 = startTimeSeconds->GetValue();
        Time appStart = slBearersActivationTime + Seconds(jitter2);
        clientFR1Apps5.Get(i)->SetStartTime(appStart);
        realAppStart = slBearersActivationTime.GetSeconds() + jitter2 +
                        ((double)udpPacketSizeControl * 8.0 / (DataRate(dataRateugvddcString).GetBitRate()));
        realAppStopTime = realAppStart + simTime.GetSeconds();
        clientFR1Apps5.Get(i)->SetStopTime(Seconds(realAppStopTime));
    }

    ApplicationContainer serverApps;
    PacketSinkHelper sidelinkSink("ns3::UdpSocketFactory", localAddress);
    sidelinkSink.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));

    ApplicationContainer serverApps2;
    PacketSinkHelper sidelinkSink2("ns3::UdpSocketFactory", localAddress2);
    sidelinkSink2.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));

    ApplicationContainer serverApps3;
    PacketSinkHelper sidelinkSink3("ns3::UdpSocketFactory", localAddress3);
    sidelinkSink3.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));

    ApplicationContainer serverFR1Apps1;
    PacketSinkHelper FR1Sink1("ns3::UdpSocketFactory", localAddress4);
    FR1Sink1.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));

    ApplicationContainer serverFR1Apps2;
    PacketSinkHelper FR1Sink2("ns3::UdpSocketFactory", localAddress5);
    FR1Sink2.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));
    
    ApplicationContainer serverFR1Apps3;
    PacketSinkHelper FR1Sink3("ns3::UdpSocketFactory", localAddress6);
    FR1Sink3.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));

    ApplicationContainer serverFR1Apps4;
    PacketSinkHelper FR1Sink4("ns3::UdpSocketFactory", localAddress7);
    FR1Sink4.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));

    ApplicationContainer serverFR1Apps5;
    PacketSinkHelper FR1Sink5("ns3::UdpSocketFactory", localAddress8);
    FR1Sink5.SetAttribute("EnableSeqTsSizeHeader", BooleanValue(true));

    for (uint32_t i = 0; i < RsuUes.GetN(); i++)
    {
        serverApps.Add(sidelinkSink.Install(RsuUes.Get(i)));
        serverApps.Start(Seconds(1.0));
    }
    for (uint32_t i = 0; i < CavUEs.GetN(); i++)
    {
        serverApps2.Add(sidelinkSink2.Install(CavUEs.Get(i)));
        serverApps2.Start(Seconds(1.0));
        serverApps3.Add(sidelinkSink3.Install(CavUEs.Get(i)));
        serverApps3.Start(Seconds(1.0));
    }
    for(uint32_t i = 0; i < DoUEs.GetN(); i++)
    {
            serverFR1Apps1.Add(FR1Sink1.Install(DoUEs.Get(i)));
            serverFR1Apps1.Start(Seconds(1.0));
        
            serverFR1Apps4.Add(FR1Sink4.Install(DoUEs.Get(i)));
            serverFR1Apps4.Start(Seconds(1.0));
    }
    for(uint32_t i = 0; i < UgvUes.GetN(); i++)
    {
        serverFR1Apps2.Add(FR1Sink2.Install(UgvUes.Get(i)));
        serverFR1Apps2.Start(Seconds(1.0));
    }
    serverFR1Apps5.Add(FR1Sink5.Install(DDCUe.Get(0)));
    serverFR1Apps5.Start(Seconds(1.0));

    serverFR1Apps3.Add(FR1Sink5.Install(DDCUe.Get(0)));
    serverFR1Apps3.Start(Seconds(1.0));

    /*
     * Hook the traces, for trace data to be stored in a database
     */
    std::string exampleName = "tesi6sect";
    // Datebase setup
    SQLiteOutput db(outputDir + exampleName + ".db");

    UeMacPscchTxOutputStats pscchStats;
    pscchStats.SetDb(&db, "pscchTxUeMac");
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/"
                                  "ComponentCarrierMapUe/*/NrUeMac/SlPscchScheduling",
                                  MakeBoundCallback(&NotifySlPscchScheduling, &pscchStats));

    UeMacPsschTxOutputStats psschStats;
    psschStats.SetDb(&db, "psschTxUeMac");
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/"
                                  "ComponentCarrierMapUe/*/NrUeMac/SlPsschScheduling",
                                  MakeBoundCallback(&NotifySlPsschScheduling, &psschStats));

    UePhyPscchRxOutputStats pscchPhyStats;
    pscchPhyStats.SetDb(&db, "pscchRxUePhy");
    Config::ConnectWithoutContext(
        "/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUePhy/"
        "SpectrumPhy/RxPscchTraceUe",
        MakeBoundCallback(&NotifySlPscchRx, &pscchPhyStats));

    UePhyPsschRxOutputStats psschPhyStats;
    psschPhyStats.SetDb(&db, "psschRxUePhy");
    Config::ConnectWithoutContext(
        "/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/ComponentCarrierMapUe/*/NrUePhy/"
        "SpectrumPhy/RxPsschTraceUe",
        MakeBoundCallback(&NotifySlPsschRx, &psschPhyStats));

    UeRlcRxOutputStats ueRlcRxStats;
    ueRlcRxStats.SetDb(&db, "rlcRx");
    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::NrUeNetDevice/"
                                  "ComponentCarrierMapUe/*/NrUeMac/RxRlcPduWithTxRnti",
                                  MakeBoundCallback(&NotifySlRlcPduRx, &ueRlcRxStats));

    UeToUePktTxRxOutputStats pktStats;
    pktStats.SetDb(&db, "pktTxRx");

    if (!useIPv6)
    {
        // Set Tx traces
        for (uint32_t ac = 0; ac < clientApps.GetN(); ac++)
        {
            Ipv4Address localAddrs = clientApps.Get(ac)
                                         ->GetNode()
                                         ->GetObject<Ipv4L3Protocol>()
                                         ->GetAddress(1, 0)
                                         .GetLocal();
            std::cout << "Tx address: " << localAddrs << std::endl;
            clientApps.Get(ac)->TraceConnect("TxWithSeqTsSize",
                                             "tx",
                                             MakeBoundCallback(&UePacketTraceDb,
                                                               &pktStats,
                                                               clientApps.Get(ac)->GetNode(),
                                                               localAddrs));
        }
        for (uint32_t ac = 0; ac < clientApps2.GetN(); ac++)
        {
            Ipv4Address localAddrs = clientApps2.Get(ac)
                                         ->GetNode()
                                         ->GetObject<Ipv4L3Protocol>()
                                         ->GetAddress(1, 0)
                                         .GetLocal();
            std::cout << "Tx address: " << localAddrs << std::endl;
            clientApps2.Get(ac)->TraceConnect("TxWithSeqTsSize",
                                             "tx",
                                             MakeBoundCallback(&UePacketTraceDb,
                                                               &pktStats,
                                                               clientApps2.Get(ac)->GetNode(),
                                                               localAddrs));
        }
        for (uint32_t ac = 0; ac < clientApps3.GetN(); ac++)
        {
            Ipv4Address localAddrs = clientApps3.Get(ac)
                                         ->GetNode()
                                         ->GetObject<Ipv4L3Protocol>()
                                         ->GetAddress(1, 0)
                                         .GetLocal();
            std::cout << "Tx address: " << localAddrs << std::endl;
            clientApps3.Get(ac)->TraceConnect("TxWithSeqTsSize",
                                             "tx",
                                             MakeBoundCallback(&UePacketTraceDb,
                                                               &pktStats,
                                                               clientApps3.Get(ac)->GetNode(),
                                                               localAddrs));
        }
        for(uint32_t ac = 0; ac < clientFR1Apps1.GetN(); ac++)
        {
            Ipv4Address localAddrs = clientFR1Apps1.Get(ac)
                                         ->GetNode()
                                         ->GetObject<Ipv4L3Protocol>()
                                         ->GetAddress(1, 0)
                                         .GetLocal();
            std::cout << "Tx address: " << localAddrs << std::endl;
            clientFR1Apps1.Get(ac)->TraceConnect("TxWithSeqTsSize",
                                             "tx",
                                             MakeBoundCallback(&UePacketTraceDb,
                                                               &pktStats,
                                                               clientFR1Apps1.Get(ac)->GetNode(),
                                                               localAddrs));
        }
        for(uint32_t ac = 0; ac < clientFR1Apps2.GetN(); ac++)
        {
            Ipv4Address localAddrs = clientFR1Apps2.Get(ac)
                                         ->GetNode()
                                         ->GetObject<Ipv4L3Protocol>()
                                         ->GetAddress(1, 0)
                                         .GetLocal();
            std::cout << "Tx address: " << localAddrs << std::endl;
            clientFR1Apps2.Get(ac)->TraceConnect("TxWithSeqTsSize",
                                             "tx",
                                             MakeBoundCallback(&UePacketTraceDb,
                                                               &pktStats,
                                                               clientFR1Apps2.Get(ac)->GetNode(),
                                                               localAddrs));
        }
        for(uint32_t ac = 0; ac < clientFR1Apps3.GetN(); ac++)
        {
            Ipv4Address localAddrs = clientFR1Apps3.Get(ac)
                                         ->GetNode()
                                         ->GetObject<Ipv4L3Protocol>()
                                         ->GetAddress(1, 0)
                                         .GetLocal();
            std::cout << "Tx address: " << localAddrs << std::endl;
            clientFR1Apps3.Get(ac)->TraceConnect("TxWithSeqTsSize",
                                             "tx",
                                             MakeBoundCallback(&UePacketTraceDb,
                                                               &pktStats,
                                                               clientFR1Apps3.Get(ac)->GetNode(),
                                                               localAddrs));
        }
        for(uint32_t ac = 0; ac < clientFR1Apps4.GetN(); ac++)
        {
            Ipv4Address localAddrs = clientFR1Apps4.Get(ac)
                                         ->GetNode()
                                         ->GetObject<Ipv4L3Protocol>()
                                         ->GetAddress(1, 0)
                                         .GetLocal();
            std::cout << "Tx address: " << localAddrs << std::endl;
            clientFR1Apps4.Get(ac)->TraceConnect("TxWithSeqTsSize",
                                             "tx",
                                             MakeBoundCallback(&UePacketTraceDb,
                                                               &pktStats,
                                                               clientFR1Apps4.Get(ac)->GetNode(),
                                                               localAddrs));
        }
        for(uint32_t ac = 0; ac < clientFR1Apps5.GetN(); ac++)
        {
            Ipv4Address localAddrs = clientFR1Apps5.Get(ac)
                                         ->GetNode()
                                         ->GetObject<Ipv4L3Protocol>()
                                         ->GetAddress(1, 0)
                                         .GetLocal();
            std::cout << "Tx address: " << localAddrs << std::endl;
            clientFR1Apps5.Get(ac)->TraceConnect("TxWithSeqTsSize",
                                             "tx",
                                             MakeBoundCallback(&UePacketTraceDb,
                                                               &pktStats,
                                                               clientFR1Apps5.Get(ac)->GetNode(),
                                                               localAddrs));
        }

        // Set Rx traces
        for (uint32_t ac = 0; ac < serverApps.GetN(); ac++)
        {
            Ipv4Address localAddrs = serverApps.Get(ac)
                                         ->GetNode()
                                         ->GetObject<Ipv4L3Protocol>()
                                         ->GetAddress(1, 0)
                                         .GetLocal();
            std::cout << "Rx address: " << localAddrs << std::endl;
            serverApps.Get(ac)->TraceConnect("RxWithSeqTsSize",
                                             "rx",
                                             MakeBoundCallback(&UePacketTraceDb,
                                                               &pktStats,
                                                               serverApps.Get(ac)->GetNode(),
                                                               localAddrs));
        }
        for (uint32_t ac = 0; ac < serverApps2.GetN(); ac++)
        {
            Ipv4Address localAddrs = serverApps2.Get(ac)
                                         ->GetNode()
                                         ->GetObject<Ipv4L3Protocol>()
                                         ->GetAddress(1, 0)
                                         .GetLocal();
            std::cout << "Rx address: " << localAddrs << std::endl;
            serverApps2.Get(ac)->TraceConnect("RxWithSeqTsSize",
                                             "rx",
                                             MakeBoundCallback(&UePacketTraceDb,
                                                               &pktStats,
                                                               serverApps2.Get(ac)->GetNode(),
                                                               localAddrs));
        }
        for (uint32_t ac = 0; ac < serverApps3.GetN(); ac++)
        {
            Ipv4Address localAddrs = serverApps3.Get(ac)
                                         ->GetNode()
                                         ->GetObject<Ipv4L3Protocol>()
                                         ->GetAddress(1, 0)
                                         .GetLocal();
            std::cout << "Rx address: " << localAddrs << std::endl;
            serverApps3.Get(ac)->TraceConnect("RxWithSeqTsSize",
                                             "rx",
                                             MakeBoundCallback(&UePacketTraceDb,
                                                               &pktStats,
                                                               serverApps3.Get(ac)->GetNode(),
                                                               localAddrs));
        }
        for(uint32_t ac = 0; ac < serverFR1Apps1.GetN(); ac++)
        {
            Ipv4Address localAddrs = serverFR1Apps1.Get(ac)
                                         ->GetNode()
                                         ->GetObject<Ipv4L3Protocol>()
                                         ->GetAddress(1, 0)
                                         .GetLocal();
            std::cout << "Rx address: " << localAddrs << std::endl;
            serverFR1Apps1.Get(ac)->TraceConnect("RxWithSeqTsSize",
                                             "rx",
                                             MakeBoundCallback(&UePacketTraceDb,
                                                               &pktStats,
                                                               serverFR1Apps1.Get(ac)->GetNode(),
                                                               localAddrs));
        }
        for(uint32_t ac = 0; ac < serverFR1Apps2.GetN(); ac++)
        {
            Ipv4Address localAddrs = serverFR1Apps2.Get(ac)
                                         ->GetNode()
                                         ->GetObject<Ipv4L3Protocol>()
                                         ->GetAddress(1, 0)
                                         .GetLocal();
            std::cout << "Rx address: " << localAddrs << std::endl;
            serverFR1Apps2.Get(ac)->TraceConnect("RxWithSeqTsSize",
                                             "rx",
                                             MakeBoundCallback(&UePacketTraceDb,
                                                               &pktStats,
                                                               serverFR1Apps2.Get(ac)->GetNode(),
                                                               localAddrs));
        }
        for(uint32_t ac = 0; ac < serverFR1Apps3.GetN(); ac++)
        {
            Ipv4Address localAddrs = serverFR1Apps3.Get(ac)
                                         ->GetNode()
                                         ->GetObject<Ipv4L3Protocol>()
                                         ->GetAddress(1, 0)
                                         .GetLocal();
            std::cout << "Rx address: " << localAddrs << std::endl;
            serverFR1Apps3.Get(ac)->TraceConnect("RxWithSeqTsSize",
                                             "rx",
                                             MakeBoundCallback(&UePacketTraceDb,
                                                               &pktStats,
                                                               serverFR1Apps3.Get(ac)->GetNode(),
                                                               localAddrs));
        }
        for(uint32_t ac = 0; ac < serverFR1Apps4.GetN(); ac++)
        {
            Ipv4Address localAddrs = serverFR1Apps4.Get(ac)
                                         ->GetNode()
                                         ->GetObject<Ipv4L3Protocol>()
                                         ->GetAddress(1, 0)
                                         .GetLocal();
            std::cout << "Rx address: " << localAddrs << std::endl;
            serverFR1Apps4.Get(ac)->TraceConnect("RxWithSeqTsSize",
                                             "rx",
                                             MakeBoundCallback(&UePacketTraceDb,
                                                               &pktStats,
                                                               serverFR1Apps4.Get(ac)->GetNode(),
                                                               localAddrs));
        }
        for(uint32_t ac = 0; ac < serverFR1Apps5.GetN(); ac++)
        {
            Ipv4Address localAddrs = serverFR1Apps5.Get(ac)
                                         ->GetNode()
                                         ->GetObject<Ipv4L3Protocol>()
                                         ->GetAddress(1, 0)
                                         .GetLocal();
            std::cout << "Rx address: " << localAddrs << std::endl;
            serverFR1Apps5.Get(ac)->TraceConnect("RxWithSeqTsSize",
                                             "rx",
                                             MakeBoundCallback(&UePacketTraceDb,
                                                               &pktStats,
                                                               serverFR1Apps5.Get(ac)->GetNode(),
                                                               localAddrs));
        }
    }
    else
    {
        // Set Tx traces
        for (uint32_t ac = 0; ac < clientApps.GetN(); ac++)
        {
            clientApps.Get(ac)->GetNode()->GetObject<Ipv6L3Protocol>()->AddMulticastAddress(
                groupAddress6);
            Ipv6Address localAddrs = clientApps.Get(ac)
                                         ->GetNode()
                                         ->GetObject<Ipv6L3Protocol>()
                                         ->GetAddress(1, 1)
                                         .GetAddress();
            std::cout << "Tx address: " << localAddrs << std::endl;
            clientApps.Get(ac)->TraceConnect("TxWithSeqTsSize",
                                             "tx",
                                             MakeBoundCallback(&UePacketTraceDb,
                                                               &pktStats,
                                                               clientApps.Get(ac)->GetNode(),
                                                               localAddrs));
        }

        // Set Rx traces
        for (uint32_t ac = 0; ac < serverApps.GetN(); ac++)
        {
            serverApps.Get(ac)->GetNode()->GetObject<Ipv6L3Protocol>()->AddMulticastAddress(
                groupAddress6);
            Ipv6Address localAddrs = serverApps.Get(ac)
                                         ->GetNode()
                                         ->GetObject<Ipv6L3Protocol>()
                                         ->GetAddress(1, 1)
                                         .GetAddress();
            std::cout << "Rx address: " << localAddrs << std::endl;
            serverApps.Get(ac)->TraceConnect("RxWithSeqTsSize",
                                             "rx",
                                             MakeBoundCallback(&UePacketTraceDb,
                                                               &pktStats,
                                                               serverApps.Get(ac)->GetNode(),
                                                               localAddrs));
        }
    }

    V2xKpi v2xKpi;
    v2xKpi.SetDbPath(outputDir + exampleName);
    v2xKpi.SetTxAppDuration(txAppDurationFin);
    SavePositionPerIP(&v2xKpi);
    v2xKpi.SetRangeForV2xKpis(50);

    if (generateInitialPosGnuScript)
    {
        std::string initPosFileName = "init-pos-ues-" + exampleName + ".txt";
        PrintUeInitPosToFile(initPosFileName);
    }

    // Final simulation stop time is the addition of: simTime + slBearersActivationTime +
    // realAppStart realAppStart is of the last UE for which we installed the application
    Time simStopTime = simTime + slBearersActivationTime + Seconds(realAppStart);

    if (generateGifGnuScript)
    {
        std::string mobilityFileName = "mobility-" + exampleName + ".txt";
        RecordMobility(true, mobilityFileName);
        WriteGifGnuScript(mobilityFileName,
                          simStopTime,
                          CAVsSpeed,
                          RsuUes.Get(0),
                          RsuUes.Get(RsuUes.GetN() - 1));
    }
    
    Simulator::Stop(simStopTime);
    Simulator::Run();

    /*
     * VERY IMPORTANT: Do not forget to empty the database cache, which would
     * dump the data store towards the end of the simulation in to a database.
     */

    pktStats.EmptyCache();
    pscchStats.EmptyCache();
    psschStats.EmptyCache();
    pscchPhyStats.EmptyCache();
    psschPhyStats.EmptyCache();
    ueRlcRxStats.EmptyCache();
    v2xKpi.WriteKpis(numSector);

    // GtkConfigStore config;
    //  config.ConfigureAttributes ();

    Simulator::Destroy();
    return 0;
}