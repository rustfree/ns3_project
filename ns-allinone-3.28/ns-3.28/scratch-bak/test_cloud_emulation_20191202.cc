

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
#include "ns3/object-vector.h"
#include "ns3/assert.h"
#include <vector>
//#include ""
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TestCloudModemUan");


void set_RealModemNodeList(){
  NS_LOG_DEBUG ("set_RealModemNodeList");
  std::vector<uint16_t> modemList = {123,124};
  UanEmulationManager * ptr = UanEmulationManager::getInstance(modemList);
  if(ptr -> checkMapIsValid(modemList) == false){
      NS_ASSERT_MSG (0, "Failed: Node id Map Not allowed");
  }else{
      NS_LOG_DEBUG ("Succeed: Node id Map is allowed!");
  }
}

int
main (int argc, char *argv[])
{
  //LogComponentEnable ("DataApplicationUanExample", LOG_LEVEL_ALL);
  LogComponentEnable ("UanPhyRealCloud", LOG_LEVEL_ALL);
  LogComponentEnable ("TestCloudModemUan", LOG_LEVEL_ALL);
  LogComponentEnable ("UanPhyGen", LOG_LEVEL_ALL);
  LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
  LogComponentEnable ("UanEmulationPacket", LOG_LEVEL_ALL);
  LogComponentEnable ("UanEmulationManager", LOG_LEVEL_ALL);
  LogComponentEnable ("UanMacAloha", LOG_LEVEL_ALL);
  LogComponentEnable ("UanNetDevice", LOG_LEVEL_ALL);




  bool EmuMode = true;
  std::string phySetStr;
  if(EmuMode == true){
    GlobalValue::Bind ("SimulatorImplementationType",
                    StringValue ("ns3::RealtimeSimulatorImpl"));//实时仿真，执行起来比较慢
    phySetStr = "ns3::UanPhyRealCloud";
  }else{
    phySetStr = "ns3::UanPhyGen";
  }

  
  int packetSize = 1000;
  int m_numNodes = 1;
  int m_dataRate = 600;//800
  int endTime = 120;

  std::string perModel = "ns3::UanPhyPerGenDefault";
  std::string sinrModel = "ns3::UanPhyCalcSinrDefault";
  std::string m_asciitracefile = "dataApplication-packet-socket_cloud.asc";
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
  uan.SetPhy (phySetStr,
              "PerModel", PointerValue (per),
              "SinrModel", PointerValue (sinr),
              "SupportedModes", UanModesListValue (myModes)
              );
  uan.SetMac ("ns3::UanMacAloha");//设置macAloha的参数
  /*创建节点*/
  NodeContainer nc = NodeContainer ();
  NodeContainer sink = NodeContainer ();//一个sink节点和x个nc节点
  nc.Create (m_numNodes);
  sink.Create (1);
  NS_LOG_INFO ("Create Node");
 

  //信道模型
  Ptr<UanPropModelIdeal> prop = CreateObjectWithAttributes<UanPropModelIdeal> ();

  /*创建uan信道*/
  Ptr<UanChannel> channel = CreateObjectWithAttributes<UanChannel> ("PropagationModel", PointerValue (prop));

  //安装UAN信道，返回创建设备。说明设备和信道是固定匹配的
  NetDeviceContainer devices = uan.Install (nc, channel);
  NetDeviceContainer sinkdev = uan.Install (sink, channel);

  /*移动模型，为每个节点分配位置信息*/
  int m_length = 100, m_width = 8, m_depth = 4;
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> pos = CreateObject<ListPositionAllocator> ();
  
 
  Ptr<UniformRandomVariable> urv = CreateObject<UniformRandomVariable> ();
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
 //发送端:nc0

  DataApplicationHelper DataAppsend ("ns3::PacketSocketFactory", Address (sinkAddress));
  //DataAppsend.SetAttribute ("Remote", destAddress1);
  DataAppsend.SetAttribute ("Mean", DoubleValue(5));
  DataAppsend.SetAttribute ("Bound", DoubleValue(12));
  DataAppsend.SetAttribute ("PacketSize", UintegerValue (packetSize));
 
  ApplicationContainer sendApps = DataAppsend.Install (nc.Get (0));//安装到节点0
  DataAppsend.SetFill(sendApps.Get(0),
"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcde");
  sendApps.Start (Seconds (2.1));
  sendApps.Stop (Seconds (endTime));
  NS_LOG_INFO ("send app");
 



 //接收端
 
  PacketSinkHelper sinkHelper ("ns3::PacketSocketFactory", Address ());
  sinkHelper.SetAttribute ("Local", AddressValue(sinkAddress));

  ApplicationContainer sinkApp = sinkHelper.Install (sink.Get (0));//server node
  
  sinkApp.Start (Seconds (2.0));
  sinkApp.Stop (Seconds (endTime+10));
  
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

  if(EmuMode == true){
    set_RealModemNodeList();
  }
  //simulate
  Simulator::Stop (Seconds (endTime+15));
  Simulator::Run ();
  Simulator::Destroy ();
}
