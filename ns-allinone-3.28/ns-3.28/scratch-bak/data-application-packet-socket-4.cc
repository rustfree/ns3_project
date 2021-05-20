
//by 2018\8\2,单个节点安装多个应用层，分别向不同节点发送, 共4个节点

#include <sys/socket.h>
#include <errno.h>
#include "ns3/stats-module.h"
#include "ns3/uan-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"

#include "ns3/rng-seed-manager.h"
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RandomSendPacketSocketExampleN4");

void InstallMobility(NodeContainer nc ,int m_numNodes);
void SetApplication(NodeContainer nc ,NetDeviceContainer ncdev,int packetSize);


double Sendtime = 800;
double Sinktime = 925;
double Simutime = 935;

int
main ()
{


  //LogComponentEnable ("RandomSendPacketSocketExample", LOG_LEVEL_ALL);
  //LogComponentEnable ("DataApplication", LOG_LEVEL_ALL);
 // LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
  //LogComponentEnable ("Ipv4L3Protocol", LOG_LEVEL_ALL);
  //LogComponentEnable ("UanMacNewAloha", LOG_LEVEL_ALL);
  //LogComponentEnable ("UanPhyGen", LOG_LEVEL_ALL);
 //LogComponentEnable ("UanChannel", LOG_LEVEL_ALL);

  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  //GlobalValue::Bind ("SimulatorImplementationType",StringValue ("ns3::RealtimeSimulatorImpl"));//实时仿真，执行起来比较慢
   RngSeedManager::SetSeed(3);//**
 
  int packetSize = 1200;//500
  
  int m_dataRate = 1000;

  std::string perModel = "ns3::UanPhyPerGenDefault";
  std::string sinrModel = "ns3::UanPhyCalcSinrDefault";
  std::string m_asciitracefile = "data-application-packet-socket-4.asc";
  /*1.物理层phy的配置*/
  //配置的信息存在uanhelper中,最终传递给run函数
  ObjectFactory obf;
  obf.SetTypeId (perModel);
  Ptr<UanPhyPer> per = obf.Create<UanPhyPer> ();
  obf.SetTypeId (sinrModel);
  Ptr<UanPhyCalcSinr> sinr = obf.Create<UanPhyCalcSinr> ();
 
  UanHelper uan;  //uan助手类
  UanTxMode mode; //发送模式
  mode = UanTxModeFactory::CreateMode (UanTxMode::PSK, m_dataRate,
                                       m_dataRate, 12000,
                                       m_dataRate, 2,
                                       "QPSK");//调制方式，datarate,symbolrate,centerfrequency,bandwidth
  UanModesList myModes;
  myModes.AppendMode (mode);
  uan.SetPhy ("ns3::UanPhyGen",
              "PerModel", PointerValue (per),
              "SinrModel", PointerValue (sinr),
              "SupportedModes", UanModesListValue (myModes));
  uan.SetMac ("ns3::UanMacNewAloha");//设置macAloha的参数
  /*2.创建节点*/
  NodeContainer nc = NodeContainer ();

  int m_numNodes = 4;
  nc.Create (m_numNodes);
 
  NS_LOG_INFO ("Create Node");

  /*3.创建uan信道*/
  //信道模型
  Ptr<UanPropModelIdeal> prop = CreateObjectWithAttributes<UanPropModelIdeal> ();
 
  Ptr<UanChannel> channel = CreateObjectWithAttributes<UanChannel> ("PropagationModel", PointerValue (prop));
 

  //安装UAN信道，返回创建设备。说明设备和信道是固定匹配的
  NetDeviceContainer ncdevices = uan.Install (nc, channel);

  NS_LOG_INFO ("channel model");

  /*4.移动模型*/
  InstallMobility( nc,m_numNodes);


  //安装PacketSocketHelper
  NS_LOG_DEBUG ("socket created");
  PacketSocketHelper socketHelper;
  socketHelper.Install (nc);     //将一个PacketSocket的实例汇聚到nc节点中



 //配置应用层
 SetApplication( nc ,ncdevices,packetSize);
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
  Simulator::Stop (Seconds (Simutime));
  Simulator::Run ();
  Simulator::Destroy ();
}



void InstallMobility(NodeContainer nc ,int m_numNodes)
{


  double m_length = 100, m_width = 8, m_depth = 4;
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> pos = CreateObject<ListPositionAllocator> ();
  
  
  Ptr<UniformRandomVariable> urv = CreateObject<UniformRandomVariable> ();
 
  double rsum = 0;

  double minr = 2 * m_length * m_width;
  //可选坐标列表
  double x_list[4] = {0, 0,m_width,m_width};
  double y_list[4] = {0, m_length ,0,m_length};
    for (int i = 0; i < m_numNodes; i++)//后面有待完善
      {
        double x = x_list[i];
        double y = y_list[i];
        double newr = std::sqrt ((x - m_length / 2.0) * (x - m_length / 2.0)
                            + (y - m_width / 2.0) * (y - m_width / 2.0));
        rsum += newr;
        minr = std::min (minr, newr);//??????
        pos->Add (Vector (x, y, m_depth));//nc的位置

      }

  /*安装移动模型*/   //位置恒定
    mobility.SetPositionAllocator (pos);//位置信息
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");//位置信息为常量

    mobility.Install (nc);

}

void SetApplication(NodeContainer nc ,NetDeviceContainer ncdev,int packetSize)
{

  //Ptr<ExponentialRandomVariable> erv;
  //erv = CreateObject<ExponentialRandomVariable> ();


  /*8.应用层*/
  //send
  //目的地址:A,B,C,D
  PacketSocketAddress ncAddressA;
  ncAddressA.SetSingleDevice ( ncdev.Get(0) -> GetIfIndex() );
  ncAddressA.SetPhysicalAddress ( ncdev.Get(0) -> GetAddress() );
  ncAddressA.SetProtocol(0);

  PacketSocketAddress ncAddressB;
  ncAddressB.SetSingleDevice ( ncdev.Get(1) -> GetIfIndex() );
  ncAddressB.SetPhysicalAddress ( ncdev.Get(1) -> GetAddress() );
  ncAddressB.SetProtocol(0);

  PacketSocketAddress ncAddressC;
  ncAddressC.SetSingleDevice ( ncdev.Get(2) -> GetIfIndex() );
  ncAddressC.SetPhysicalAddress ( ncdev.Get(2) -> GetAddress() );
  ncAddressC.SetProtocol(0);

  PacketSocketAddress ncAddressD;
  ncAddressD.SetSingleDevice ( ncdev.Get(3) -> GetIfIndex() );
  ncAddressD.SetPhysicalAddress ( ncdev.Get(3) -> GetAddress() );
  ncAddressD.SetProtocol(0);


  DataApplicationHelper DataAppsend ("ns3::PacketSocketFactory", Address ());
  DataAppsend.SetAttribute ("Mean", DoubleValue(60));
  DataAppsend.SetAttribute ("Bound", DoubleValue(160));
  DataAppsend.SetAttribute ("PacketSize", UintegerValue (packetSize));

  //应用层：ncA
  //app1-->ncB
  DataAppsend.SetAttribute ("Remote", AddressValue(ncAddressB));
  ApplicationContainer sendAppsA1 = DataAppsend.Install (nc.Get (0));//安装到节点ncA
  DataAppsend.SetFill(sendAppsA1.Get(0),
"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcde");
  sendAppsA1.Start (Seconds (10));
  sendAppsA1.Stop (Seconds (Sendtime));
  //app2-->ncC
  DataAppsend.SetAttribute ("Remote", AddressValue(ncAddressC));
  ApplicationContainer sendAppsA2 = DataAppsend.Install (nc.Get (0));
  DataAppsend.SetFill(sendAppsA2.Get(0),
"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcd");
  sendAppsA2.Start (Seconds (10));
  sendAppsA2.Stop (Seconds (Sendtime));
   //app3-->ncD
  DataAppsend.SetAttribute ("Remote", AddressValue(ncAddressD));
  ApplicationContainer sendAppsA3 = DataAppsend.Install (nc.Get (0));
  DataAppsend.SetFill(sendAppsA3.Get(0),
"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcd");
  sendAppsA3.Start (Seconds (10));
  sendAppsA3.Stop (Seconds (Sendtime));

  NS_LOG_INFO ("send app");

  //应用层：ncB

  //app1-->ncA
  DataAppsend.SetAttribute ("Remote", AddressValue(ncAddressA));
  ApplicationContainer sendAppsB1 = DataAppsend.Install (nc.Get (1));//安装到节点ncB
  DataAppsend.SetFill(sendAppsB1.Get(0),
"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcde");
  sendAppsB1.Start (Seconds (10));
  sendAppsB1.Stop (Seconds (Sendtime));
  //app2-->ncC
  DataAppsend.SetAttribute ("Remote", AddressValue(ncAddressC));
  ApplicationContainer sendAppsB2 = DataAppsend.Install (nc.Get (1));
  DataAppsend.SetFill(sendAppsB2.Get(0),
"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcd");
  sendAppsB2.Start (Seconds (10));
  sendAppsB2.Stop (Seconds (Sendtime));
   //app3-->ncD
  DataAppsend.SetAttribute ("Remote", AddressValue(ncAddressD));
  ApplicationContainer sendAppsB3 = DataAppsend.Install (nc.Get (1));
  DataAppsend.SetFill(sendAppsB3.Get(0),
"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcd");
  sendAppsB3.Start (Seconds (10));
  sendAppsB3.Stop (Seconds (Sendtime));


  //应用层：ncC

  //app1-->ncA
  DataAppsend.SetAttribute ("Remote", AddressValue(ncAddressA));
  ApplicationContainer sendAppsC1 = DataAppsend.Install (nc.Get (2));//安装到节点ncC
  DataAppsend.SetFill(sendAppsC1.Get(0),
"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcde");
  sendAppsC1.Start (Seconds (10));
  sendAppsC1.Stop (Seconds (Sendtime));

  //app2-->ncB
  DataAppsend.SetAttribute ("Remote", AddressValue(ncAddressB));
  ApplicationContainer sendAppsC2 = DataAppsend.Install (nc.Get (2));//安装到节点ncC
  DataAppsend.SetFill(sendAppsC2.Get(0),
"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcde");
  sendAppsC2.Start (Seconds (10));
  sendAppsC2.Stop (Seconds (Sendtime));
 //app3-->ncD
  DataAppsend.SetAttribute ("Remote", AddressValue(ncAddressD));
  ApplicationContainer sendAppsC3 = DataAppsend.Install (nc.Get (2));
  DataAppsend.SetFill(sendAppsC3.Get(0),
"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcd");
  sendAppsC3.Start (Seconds (10));
  sendAppsC3.Stop (Seconds (Sendtime));

  //应用层：ncD

  //app1-->ncA
  DataAppsend.SetAttribute ("Remote", AddressValue(ncAddressA));
  ApplicationContainer sendAppsD1 = DataAppsend.Install (nc.Get (3));//安装到节点ncD
  DataAppsend.SetFill(sendAppsD1.Get(0),
"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcde");
  sendAppsD1.Start (Seconds (10));
  sendAppsD1.Stop (Seconds (Sendtime));

  //app2-->ncB
  DataAppsend.SetAttribute ("Remote", AddressValue(ncAddressB));
  ApplicationContainer sendAppsD2 = DataAppsend.Install (nc.Get (3));//安装到节点ncD
  DataAppsend.SetFill(sendAppsD2.Get(0),
"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcde");
  sendAppsD2.Start (Seconds (10));
  sendAppsD2.Stop (Seconds (Sendtime));

  //app3-->ncC
  DataAppsend.SetAttribute ("Remote", AddressValue(ncAddressC));
  ApplicationContainer sendAppsD3 = DataAppsend.Install (nc.Get (3));//安装到节点ncD
  DataAppsend.SetFill(sendAppsD3.Get(0),
"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcde");
  sendAppsD3.Start (Seconds (10));
  sendAppsD3.Stop (Seconds (Sendtime));

  //应用层：接收端
  PacketSinkHelper sinkHelper ("ns3::PacketSocketFactory", Address ());
//ncA
  sinkHelper.SetAttribute ("Local", AddressValue(ncAddressA));
  ApplicationContainer sinkAppA = sinkHelper.Install (nc.Get (0));//server node
  
  sinkAppA.Start (Seconds (2.0));
  sinkAppA.Stop (Seconds (Sinktime));
//ncB
  sinkHelper.SetAttribute ("Local", AddressValue(ncAddressB));
  ApplicationContainer sinkAppB = sinkHelper.Install (nc.Get (1));//server node
  
  sinkAppB.Start (Seconds (2.1));
  sinkAppB.Stop (Seconds (Sinktime));
//ncC
  sinkHelper.SetAttribute ("Local", AddressValue(ncAddressC));
  ApplicationContainer sinkAppC = sinkHelper.Install (nc.Get (2));//server node
  
  sinkAppC.Start (Seconds (2.2));
  sinkAppC.Stop (Seconds (Sinktime));

//ncD
  sinkHelper.SetAttribute ("Local", AddressValue(ncAddressD));
  ApplicationContainer sinkAppD = sinkHelper.Install (nc.Get (3));//server node
  
  sinkAppD.Start (Seconds (2.2));
  sinkAppD.Stop (Seconds (Sinktime));


  
  NS_LOG_INFO ("recv app");
  
 }

