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

NS_LOG_COMPONENT_DEFINE ("RoutingUanExampleN4");

void InstallMobility(NodeContainer nc ,NodeContainer router,int m_numNodes);
void SetRoute(NodeContainer nc ,NodeContainer router);

double Sendtime = 800;
double Sinktime = 925;
double Simutime = 935;

int
main ()
{
  //LogComponentEnable ("RoutingUanExampleN4", LOG_LEVEL_ALL);
  //LogComponentEnable ("DataApplication", LOG_LEVEL_ALL);
  //LogComponentEnable ("PacketSink", LOG_LEVEL_ALL);
  //LogComponentEnable ("Ipv4L3Protocol", LOG_LEVEL_ALL);
  //LogComponentEnable ("UanMacNewAloha", LOG_LEVEL_ALL);
  //LogComponentEnable ("ArpL3Protocol", LOG_LEVEL_ALL);
  //LogComponentEnable ("UanPhyRealNew", LOG_LEVEL_ALL);

 // GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

 // GlobalValue::Bind ("SimulatorImplementationType",
                     //StringValue ("ns3::RealtimeSimulatorImpl"));//实时仿真，执行起来比较慢
 
  RngSeedManager::SetSeed(1);
  RngSeedManager::SetRun(1);
  int packetSize = 500;
  
  int m_dataRate = 2000;

  std::string perModel = "ns3::UanPhyPerGenDefault";
  std::string sinrModel = "ns3::UanPhyCalcSinrDefault";
  std::string m_asciitracefile = "routing-uan-example-4.asc";
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
  uan.SetMac ("ns3::UanMacAloha");//设置macAloha的参数
  /*2.创建节点*/
  NodeContainer nc = NodeContainer ();
  NodeContainer nc0 = NodeContainer ();
  NodeContainer nc1 = NodeContainer ();
  NodeContainer router = NodeContainer ();//一个router节点和3个nc节点
  int m_numNodes = 3;
  nc0.Create (2);
  nc1.Create (1);
  nc.Add(nc0);
  nc.Add(nc1);
  router.Create (1);
  NS_LOG_INFO ("Create Node");

  /*3.创建uan信道*/
  //信道模型
  Ptr<UanPropModelIdeal> prop = CreateObjectWithAttributes<UanPropModelIdeal> ();
 
  Ptr<UanChannel> channel = CreateObjectWithAttributes<UanChannel> ("PropagationModel", PointerValue (prop));
 

  //安装UAN信道，返回创建设备。说明设备和信道是固定匹配的
  NetDeviceContainer nc0devices = uan.Install (nc0, channel);
  NetDeviceContainer nc1devices = uan.Install (nc1, channel);
  NetDeviceContainer routerdev = uan.Install (router, channel);
  NS_LOG_INFO ("channel model");

  /*4.移动模型*/
  InstallMobility(router, nc,m_numNodes);

  /*5.安装协议栈*/
  InternetStackHelper stack;
  stack.Install (router);
  stack.Install (nc0);
  stack.Install (nc1);
  NS_LOG_INFO ("Add Internet Stack");

  //配置IP地址
  Ipv4AddressHelper address;
  address.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer nc0Interfaces, nc1Interfaces, routerInterfaces;

  routerInterfaces = address.Assign (routerdev);//10.1.2.1
  nc0Interfaces = address.Assign (nc0devices);
  
  address.SetBase ("10.1.3.0", "255.255.255.0");
  nc1Interfaces = address.Assign (nc1devices);
  NS_LOG_INFO ("Assign IP Address");
  //6.设置ARP时限,IP层协议
  NodeContainer::Iterator node = router.Begin ();
  
  while (node != router.End ())
    {
      (*node)->GetObject<Ipv4L3Protocol> ()->GetInterface (1)->GetArpCache ()->SetWaitReplyTimeout (Seconds (18));
      (*node)->GetObject<Ipv4L3Protocol> ()->GetInterface (1)->GetArpCache ()->SetAliveTimeout (Seconds (800));
      node++;
    }
  node = nc.Begin ();
  while (node != nc.End ())
    {
      (*node)->GetObject<Ipv4L3Protocol> ()->GetInterface (1)->GetArpCache ()->SetWaitReplyTimeout (Seconds (18));
      (*node)->GetObject<Ipv4L3Protocol> ()->GetInterface (1)->GetArpCache ()->SetAliveTimeout (Seconds (800));
      node++;
    }
  NS_LOG_INFO ("Set ARP Timeout");


   //7.配置静态路由,设置子网
  SetRoute(nc, router);
  /*8.应用层*/
  //send
  AddressValue destAddress1 (InetSocketAddress (nc1Interfaces.GetAddress (0), 9));//目的节点为nc2，10.1.3.1

  DataApplicationHelper DataAppsend ("ns3::UdpSocketFactory", Address ());
  DataAppsend.SetAttribute ("Remote", destAddress1);
  DataAppsend.SetAttribute ("Mean", DoubleValue(35));
  DataAppsend.SetAttribute ("Bound", DoubleValue(65));
  DataAppsend.SetAttribute ("PacketSize", UintegerValue (packetSize));
 
  //nc0_1 --> nc1
  ApplicationContainer sendApps = DataAppsend.Install (nc.Get (0));//安装到节点nc0_1,10.1.2.2
  DataAppsend.SetFill(sendApps.Get(0),
"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcde");
  sendApps.Start (Seconds (2.1));
  sendApps.Stop (Seconds (Sendtime));
  NS_LOG_INFO ("send app");

  //nc0_2 --> nc1
  ApplicationContainer sendApps1 = DataAppsend.Install (nc.Get (1));//安装到节点nc0_2,10.1.2.3
  DataAppsend.SetFill(sendApps1.Get(0),
"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcde");
  sendApps1.Start (Seconds (2.1));
  sendApps1.Stop (Seconds (Sendtime));
  NS_LOG_INFO ("send app");


  //recv
  Address sinkLocalAddress (InetSocketAddress (nc1Interfaces.GetAddress (0), 9));

  PacketSinkHelper sinkHelper ("ns3::UdpSocketFactory", sinkLocalAddress);
  ApplicationContainer sinkApp = sinkHelper.Install (nc.Get (2));//server node
  sinkApp.Start (Seconds (2.0));
  sinkApp.Stop (Seconds (Sinktime));
  
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


  //设置静态ARP
  nc0.Get(0) -> GetObject<ArpL3Protocol> ()-> AddArpEntry(nc0devices.Get(0),routerdev.Get(0)->GetAddress(),routerInterfaces.GetAddress(0));
  nc0.Get(1) -> GetObject<ArpL3Protocol> ()-> AddArpEntry(nc0devices.Get(1),routerdev.Get(0)->GetAddress(),routerInterfaces.GetAddress(0));
  router.Get(0) -> GetObject<ArpL3Protocol> ()-> AddArpEntry(routerdev.Get(0),nc1devices.Get(0)->GetAddress(),nc1Interfaces.GetAddress(0));


  NS_LOG_INFO ("Set ARP static entry");

  //simulate
  Simulator::Stop (Seconds (Simutime));
  Simulator::Run ();
  Simulator::Destroy ();
}



void InstallMobility(NodeContainer nc ,NodeContainer router,int m_numNodes)
{


  double m_length = 100, m_width = 8, m_depth = 4;
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> pos = CreateObject<ListPositionAllocator> ();
  
  
  Ptr<UniformRandomVariable> urv = CreateObject<UniformRandomVariable> ();
  pos->Add (Vector (m_length / 2.0, m_width / 2.0, m_depth));//router的位置
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
    mobility.Install (router);
    NS_LOG_DEBUG ("Position of router: "
                  << router.Get (0)->GetObject<MobilityModel> ()->GetPosition ());
    mobility.Install (nc);


}

void SetRoute(NodeContainer nc ,NodeContainer router)
{
  Ptr<Ipv4StaticRouting> staticRouting;
  //router:10.1.2.1
  staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> ((router.Get(0))->GetObject<Ipv4> ()->GetRoutingProtocol ());
  //staticRouting->SetDefaultRoute ("10.1.2.2", 0);
  staticRouting->AddNetworkRouteTo("10.1.2.0","255.255.255.0",1,0);
  staticRouting->AddNetworkRouteTo("10.1.3.0","255.255.255.0",1,0);

  //nc0_1:10.1.2.2
  staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> ((nc.Get(0))->GetObject<Ipv4> ()->GetRoutingProtocol ());
  staticRouting->SetDefaultRoute ("10.1.2.1", 1);
  staticRouting->AddNetworkRouteTo("10.1.2.0","255.255.255.0",1,0);

  //nc0_2:10.1.2.3
  staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> ((nc.Get(1))->GetObject<Ipv4> ()->GetRoutingProtocol ());
  staticRouting->SetDefaultRoute ("10.1.2.1", 1);
  staticRouting->AddNetworkRouteTo("10.1.2.0","255.255.255.0",1,0);

  //nc1:10.1.3.1
  staticRouting = Ipv4RoutingHelper::GetRouting <Ipv4StaticRouting> ((nc.Get(2))->GetObject<Ipv4> ()->GetRoutingProtocol ());
  staticRouting->SetDefaultRoute ("10.1.2.1", 1);
  staticRouting->AddNetworkRouteTo("10.1.3.0","255.255.255.0",1,0);
  
  //Ipv4GlobalRoutingHelper::PopulateRoutingTables ();//使能路由转发功能
}
