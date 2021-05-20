
//by 2018\7\14,单个节点安装多个应用层，分别向不同节点发送

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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("RandomSendExample");

void InstallMobility(NodeContainer nc ,NodeContainer sink,int m_numNodes);
void SetApplication(NodeContainer nc ,Ipv4InterfaceContainer ncInterfaces,int packetSize);

int
main ()
{
 // LogComponentEnable ("RoutingUanExample", LOG_LEVEL_ALL);
 // LogComponentEnable ("DataApplication", LOG_LEVEL_ALL);
  LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
  //LogComponentEnable ("Ipv4L3Protocol", LOG_LEVEL_ALL);
  //LogComponentEnable ("UanMacPureAloha", LOG_LEVEL_ALL);
 // LogComponentEnable ("UanPhyGen", LOG_LEVEL_ALL);
 //LogComponentEnable ("UanChannel", LOG_LEVEL_ALL);

 // GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  //GlobalValue::Bind ("SimulatorImplementationType",
                     //StringValue ("ns3::RealtimeSimulatorImpl"));//实时仿真，执行起来比较慢

 
  int packetSize = 1000;
  
  int m_dataRate = 2000;

  std::string perModel = "ns3::UanPhyPerGenDefault";
  std::string sinrModel = "ns3::UanPhyCalcSinrDefault";
  std::string m_asciitracefile = "random-send-example.asc";
  /*1.物理层phy的配置*/
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
  uan.SetPhy ("ns3::UanPhyGen",
              "PerModel", PointerValue (per),
              "SinrModel", PointerValue (sinr),
              "SupportedModes", UanModesListValue (myModes));
  uan.SetMac ("ns3::UanMacPureAloha");//设置macAloha的参数
  /*2.创建节点*/
  NodeContainer nc = NodeContainer ();
  NodeContainer sink = NodeContainer ();//一个sink节点和3个nc节点
  int m_numNodes = 4;
  nc.Create (m_numNodes);
  sink.Create (1);
  NS_LOG_INFO ("Create Node");

  /*3.创建uan信道*/
  //信道模型
  Ptr<UanPropModelIdeal> prop = CreateObjectWithAttributes<UanPropModelIdeal> ();
 
  Ptr<UanChannel> channel = CreateObjectWithAttributes<UanChannel> ("PropagationModel", PointerValue (prop));
 

  //安装UAN信道，返回创建设备。说明设备和信道是固定匹配的
  NetDeviceContainer ncdevices = uan.Install (nc, channel);
  NetDeviceContainer sinkdev = uan.Install (sink, channel);
  NS_LOG_INFO ("channel model");

  /*4.移动模型*/
  InstallMobility(sink, nc,m_numNodes);

  /*5.安装协议栈*/
  InternetStackHelper stack;
  stack.Install (sink);
  stack.Install (nc);
  NS_LOG_INFO ("Add Internet Stack");

  //配置IP地址
  Ipv4AddressHelper address;
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer ncInterfaces, sinkInterfaces;

  
  ncInterfaces = address.Assign (ncdevices);
  sinkInterfaces = address.Assign (sinkdev);//10.1.2.5
  NS_LOG_INFO ("Assign IP Address");
  //6.设置ARP时限,IP层协议
  NodeContainer::Iterator node = sink.Begin ();
  
  while (node != sink.End ())
    {
      (*node)->GetObject<Ipv4L3Protocol> ()->GetInterface (1)->GetArpCache ()->SetWaitReplyTimeout (Seconds (8));
      node++;
    }
  node = nc.Begin ();
  while (node != nc.End ())
    {
      (*node)->GetObject<Ipv4L3Protocol> ()->GetInterface (1)->GetArpCache ()->SetWaitReplyTimeout (Seconds (8));
      node++;
    }
  NS_LOG_INFO ("Set ARP Timeout");

 //配置应用层
 SetApplication( nc ,ncInterfaces,packetSize);
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
  Simulator::Stop (Seconds (441.0));
  Simulator::Run ();
  Simulator::Destroy ();
}



void InstallMobility(NodeContainer nc ,NodeContainer sink,int m_numNodes)
{


  double m_length = 100, m_width = 8, m_depth = 4;
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> pos = CreateObject<ListPositionAllocator> ();
  
  
  Ptr<UniformRandomVariable> urv = CreateObject<UniformRandomVariable> ();
  pos->Add (Vector (m_length / 2.0, m_width / 2.0, m_depth));//sink的位置
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
    mobility.Install (sink);
    NS_LOG_DEBUG ("Position of sink: "
                  << sink.Get (0)->GetObject<MobilityModel> ()->GetPosition ());
    mobility.Install (nc);

}

void SetApplication(NodeContainer nc ,Ipv4InterfaceContainer ncInterfaces,int packetSize)
{
  //获取时间种子

  /*8.应用层*/
  //send
  AddressValue destAddressA (InetSocketAddress (ncInterfaces.GetAddress (0), 9));//目的节点A：10.1.2.1
  AddressValue destAddressB (InetSocketAddress (ncInterfaces.GetAddress (1), 9));//目的节点B：10.1.2.2
  AddressValue destAddressC (InetSocketAddress (ncInterfaces.GetAddress (2), 9));//目的节点C：10.1.2.3
  AddressValue destAddressD (InetSocketAddress (ncInterfaces.GetAddress (3), 9));//目的节点D：10.1.2.4

  DataApplicationHelper DataAppsend ("ns3::UdpSocketFactory", Address ());
  DataAppsend.SetAttribute ("Mean", DoubleValue(15));
  DataAppsend.SetAttribute ("Bound", DoubleValue(150));
  DataAppsend.SetAttribute ("PacketSize", UintegerValue (packetSize));
  //app1-->ncB
  DataAppsend.SetAttribute ("Remote", destAddressB);
  ApplicationContainer sendAppsA1 = DataAppsend.Install (nc.Get (0));//安装到节点ncA,10.1.2.1
  DataAppsend.SetFill(sendAppsA1.Get(0),
"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcde");
  sendAppsA1.Start (Seconds (2.1));
  sendAppsA1.Stop (Seconds (400.0));
  //app2-->ncC
  DataAppsend.SetAttribute ("Remote", destAddressC);
  ApplicationContainer sendAppsA2 = DataAppsend.Install (nc.Get (0));
  DataAppsend.SetFill(sendAppsA2.Get(0),
"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcd");
  sendAppsA2.Start (Seconds (2.3));
  sendAppsA2.Stop (Seconds (300.0));
  //app3-->ncD
/*
  DataAppsend.SetAttribute ("Remote", destAddressD);
  ApplicationContainer sendAppsA3 = DataAppsend.Install (nc.Get (0));
  DataAppsend.SetFill(sendAppsA3.Get(0),
"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabc");
  sendAppsA3.Start (Seconds (2.1));
  sendAppsA3.Stop (Seconds (300.0));*/

  NS_LOG_INFO ("send app");
  //recv
  Address sinkLocalAddress (InetSocketAddress (ncInterfaces.GetAddress (1), 9));

  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApp = sinkHelper.Install (nc.Get (1));//server node
  sinkApp.Start (Seconds (2.0));
  sinkApp.Stop (Seconds (315.1));

  Address sinkLocalAddressC (InetSocketAddress (ncInterfaces.GetAddress (2), 9));
  PacketSinkHelper sinkHelperC ("ns3::UdpSocketFactory", sinkLocalAddressC);
  ApplicationContainer sinkAppC = sinkHelperC.Install (nc.Get (2));//server node
  sinkAppC.Start (Seconds (2.0));
  sinkAppC.Stop (Seconds (315.1));
  
  NS_LOG_INFO ("recv app");
}
/*
  Address sinkLocalAddressC (InetSocketAddress (ncInterfaces.GetAddress (2), 9));
  PacketSinkHelper sinkHelperC ("ns3::UdpSocketFactory", sinkLocalAddressC);
  ApplicationContainer sinkAppC = sinkHelperC.Install (nc.Get (2));//server node
  sinkAppC.Start (Seconds (2.0));
  sinkAppC.Stop (Seconds (345.1));*/
