

#include <sys/socket.h>
#include <errno.h>
//UDP mode
#include "ns3/stats-module.h"
#include "ns3/uan-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/basic-energy-source-helper.h"
#include "ns3/acoustic-modem-energy-model-helper.h"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("UanPacketSocketNc");

int
main ()
{
  LogComponentEnable ("UanPacketSocketNc", LOG_LEVEL_ALL);
  //LogComponentEnable ("UanPhyRealNewb", LOG_LEVEL_ALL);
  LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
  //LogComponentEnable ("UanTransducerHd", LOG_LEVEL_ALL);
  LogComponentEnable ("UanMacMacaBi", LOG_LEVEL_ALL);
  //LogComponentEnable ("UanPhyGen", LOG_LEVEL_ALL);
  //LogComponentEnable ("UanChannel", LOG_LEVEL_ALL);

  bool emu = true;
 
  RngSeedManager::SetSeed(4);//**

  int packetSize = 251;
  int m_numNodes = 4;
  int m_dataRate = 1000;
  double startTime = 2, endTime = 540;
  double meanInterval_A = 10;
  double meanInterval_B = meanInterval_A;

  double meanAppRate_A = 250*8/meanInterval_A;
  double meanAppRate_B = 250*8/meanInterval_B;
  
  double leftTime = 10;

  std::string perModel = "ns3::UanPhyPerGenDefault";
  std::string sinrModel = "ns3::UanPhyCalcSinrDefault";
  std::string m_asciitracefile = "UanPacketSocketMacabi_Nc10_ns.asc";
  /*物理层phy的配置*/
  //配置的信息存在uanhelper中,最终传递给run函数
  ObjectFactory obf;
  obf.SetTypeId (perModel);
  Ptr<UanPhyPer> per = obf.Create<UanPhyPer> ();
  obf.SetTypeId (sinrModel);
  Ptr<UanPhyCalcSinr> sinr = obf.Create<UanPhyCalcSinr> ();
 
  UanHelper uan;  //uan助手类
  UanTxMode mode; //发送模式
  mode = UanTxModeFactory::CreateMode (UanTxMode::FSK, m_dataRate,
                                       m_dataRate, 12000,
                                       m_dataRate, 2,
                                       "Default mode");//调制方式，datarate,symbolrate,centerfrequency,bandwidth
  UanModesList myModes;
  myModes.AppendMode (mode);
  /*设置物理层*/

  if(emu){
      GlobalValue::Bind ("SimulatorImplementationType",
                     StringValue ("ns3::RealtimeSimulatorImpl"));//实时仿真，执行起来比较慢
      uan.SetPhy ("ns3::UanPhyRealNewb",
              "PerModel", PointerValue (per),
              "SinrModel", PointerValue (sinr),
              "SupportedModes", UanModesListValue (myModes));
  }else{
      uan.SetPhy ("ns3::UanPhyGen",
              "PerModel", PointerValue (per),
              "SinrModel", PointerValue (sinr),
              "SupportedModes", UanModesListValue (myModes));
  }
  uan.SetMac ("ns3::UanMacMacaBi");//设置macAloha的参数
  /*创建节点*/
  NodeContainer nc = NodeContainer ();
  NodeContainer sink = NodeContainer ();//一个sink节点和x个nc节点
  nc.Create (m_numNodes);//two nc, one sink
  sink.Create (1);
  NS_LOG_INFO ("Create Node");

  //能量模型设置
  BasicEnergySourceHelper energySourceHelper;
  energySourceHelper.Set ("BasicEnergySourceInitialEnergyJ", DoubleValue (80000));
  energySourceHelper.Install (sink);
  energySourceHelper.Install (nc);


  //信道模型
  Ptr<UanPropModelIdeal> prop = CreateObjectWithAttributes<UanPropModelIdeal> ();

  /*创建uan信道*/
  Ptr<UanChannel> channel = CreateObjectWithAttributes<UanChannel> ("PropagationModel", PointerValue (prop));

  //安装UAN信道，返回创建设备。说明设备和信道是固定匹配的
  NetDeviceContainer devices = uan.Install (nc, channel);
  NetDeviceContainer sinkdev = uan.Install (sink, channel);

  //能量模型安装
  EnergySourceContainer energySourceContainer_sink;
  NodeContainer::Iterator node = sink.Begin ();
  while (node != sink.End ())
    {
      energySourceContainer_sink.Add ((*node)->GetObject<EnergySourceContainer> ()->Get (0));
      node++;
    }
  AcousticModemEnergyModelHelper acousticModemEnergyModelHelper;
  acousticModemEnergyModelHelper.Install (sinkdev, energySourceContainer_sink);

  EnergySourceContainer energySourceContainer_nc;
  node = nc.Begin ();
  while (node != nc.End ())
  {
    energySourceContainer_nc.Add ((*node)->GetObject<EnergySourceContainer> ()->Get (0));
    node++;
  }

  acousticModemEnergyModelHelper.Install (devices, energySourceContainer_nc);
  

  /*移动模型，为每个节点分配位置信息*/
  int m_length = 100, m_width = 8, m_depth = 4;
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> pos = CreateObject<ListPositionAllocator> ();
  
 
  //Ptr<UniformRandomVariable> urv = CreateObject<UniformRandomVariable> ();
  
  pos->Add (Vector (m_length / 2.0, m_width / 2.0, m_depth));//sink的位置
  double rsum = 0;

  double minr = 2 * m_length * m_width;
  for (int i = 0; i < m_numNodes; i++)//后面有待完善
  {
    double x = 0;
    double y = 0;
    double newr = std::sqrt ((x - m_length / 2.0) * (x - m_length / 2.0)
                        + (y - m_width / 2.0) * (y - m_width / 2.0));
    rsum += newr;
    minr = std::min (minr, newr);//??????
    pos->Add (Vector (x, y, m_depth));//nc的位置

  }
  /*安装移动模型*/   //位置恒定
  mobility.SetPositionAllocator (pos);//位置信息
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");//位置信息为常量
  mobility.Install (sink);
  NS_LOG_DEBUG ("Position of sink: "
                << sink.Get (0)->GetObject<MobilityModel> ()->GetPosition ());
  mobility.Install (nc);

  //目的地址
  PacketSocketAddress sinkAddress;
  sinkAddress.SetSingleDevice ( sinkdev.Get(0) -> GetIfIndex() );
  sinkAddress.SetPhysicalAddress ( sinkdev.Get(0) -> GetAddress() );
  sinkAddress.SetProtocol(0);
  
  //安装PacketSocketHelper
  NS_LOG_DEBUG ("socket created");
  PacketSocketHelper socketHelper;
  socketHelper.Install (nc);     //将一个PacketSocket的实例汇聚到nc节点中
  socketHelper.Install (sink);   //将一个PacketSocket的实例汇聚到sink节点中

 //发送端:nc0到nc10
  OnOffHelper app_A ("ns3::PacketSocketFactory", Address (sinkAddress));
  app_A.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  app_A.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  app_A.SetAttribute ("DataRate", DataRateValue (meanAppRate_A));
  app_A.SetAttribute ("PacketSize", UintegerValue (packetSize));

  OnOffHelper app_B ("ns3::PacketSocketFactory", Address (sinkAddress));
  app_B.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  app_B.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  app_B.SetAttribute ("DataRate", DataRateValue (meanAppRate_B));
  app_B.SetAttribute ("PacketSize", UintegerValue (packetSize));

  vector< ApplicationContainer> sendAppsVec;
  for(int index = 0; index < m_numNodes; index++){
      if(index < 2)
          sendAppsVec.push_back(app_A.Install (nc.Get (index)));//安装到节点0到节点9
      else
          sendAppsVec.push_back(app_B.Install (nc.Get (index)));//安装到节点0到节点9
      sendAppsVec[index].Start (Seconds (startTime ));
      sendAppsVec[index].Stop (Seconds (endTime+10));
  }
 
 //接收端
 
  PacketSinkHelper sinkHelper ("ns3::PacketSocketFactory", Address ());
  sinkHelper.SetAttribute ("Local", AddressValue(sinkAddress));

  ApplicationContainer sinkApp = sinkHelper.Install (sink.Get (0));//server node
  
  sinkApp.Start (Seconds (startTime));
  sinkApp.Stop (Seconds (endTime + leftTime  +3));
  
  NS_LOG_INFO ("recv app");

  //tracing
  std::ofstream ascii (m_asciitracefile.c_str ());
  if (!ascii.is_open ())
  {
      NS_FATAL_ERROR ("Could not open ascii trace file: "
                    << m_asciitracefile);
  }
  uan.EnableAsciiAll (ascii);
  NS_LOG_INFO ("Enable tracing");
  //simulate
  Simulator::Stop (Seconds (endTime + leftTime +5));
  Simulator::Run ();

  double energyFraction, totalEnergyUsed = 0;
  node = sink.Begin ();
  while (node != sink.End ())
  {
      energyFraction = (*node)->GetObject<EnergySourceContainer> ()->Get (0)->GetEnergyFraction();
      totalEnergyUsed += (1.0- energyFraction);
      NS_LOG_UNCOND ( "Node id:" <<(*node)->GetId() <<"-- Energy Fraction:" << energyFraction );
      node++;
      
  }
  node = nc.Begin ();
  while (node != nc.End ())
  {
      energyFraction = (*node)->GetObject<EnergySourceContainer> ()->Get (0)->GetEnergyFraction();
      totalEnergyUsed += (1.0- energyFraction);
      NS_LOG_UNCOND ( "Node id:" <<(*node)->GetId() <<"-- Energy Fraction:" << energyFraction );
      node++;
      
  }
  NS_LOG_UNCOND ( "Total Energy Used Fraction:" << totalEnergyUsed );
  Simulator::Destroy ();
}
