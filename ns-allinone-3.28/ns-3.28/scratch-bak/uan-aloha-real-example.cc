/*
*  uan-aloha-example.cc
*  Created on: 2018/4/11
*   Author:lch
*
*/

/*

node   num    c/s     id   ip
nc      1     send    0    192.168.2.101
sink    1     recv    1    192.168.2.102

*/
#include "uan-aloha-real-example.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/stats-module.h"
#include "ns3/applications-module.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("UanAlohaReal");



Experiment::Experiment () 
  : m_numNodes (2),
    m_dataRate (1000),
    m_depth (50),
    m_boundary (1000),
    m_packetSize (1020),
    m_bytesTotal (0),
    m_simTime (Seconds (12)),
    m_gnudatfile ("uan-aloha-example.gpl"),
    m_asciitracefile ("uan-aloha-real-example.asc"),
    m_bhCfgFile ("uan-apps/dat/default.cfg")
{
}

void
Experiment::ResetData ()//保存从单个run()中得到的吞吐量
{
  NS_LOG_DEBUG (Simulator::Now ().GetSeconds () << "  Resetting data");
  m_throughputs.push_back (m_bytesTotal * 8.0 / m_simTime.GetSeconds ());
  m_bytesTotal = 0;
}

void
Experiment::ReceivePacket (Ptr<Socket> socket)//接收到包，并累加计算吞吐量
{
  Ptr<Packet> packet;

  while ((packet = socket->Recv ()))
    {
      m_bytesTotal += packet->GetSize ();
    }
  packet = 0;
}

int                      //返回值为二维的点集
Experiment::Run (UanHelper &uan)
{
  NS_LOG_DEBUG ("Experiment::run");
 uan.SetMac ("ns3::UanMacAloha");//设置macAloha的参数
 
  /*创建节点*/ 
  NS_LOG_DEBUG ("node created");
  NodeContainer nc = NodeContainer ();
  NodeContainer sink = NodeContainer ();//一个sink节点和x个nc节点
  nc.Create (m_numNodes);
  sink.Create (1);

  //安装PacketSocketHelper
  NS_LOG_DEBUG ("socket created");
  PacketSocketHelper socketHelper;
  socketHelper.Install (nc);     //将一个PacketSocket的实例汇聚到nc节点中
  socketHelper.Install (sink);   //将一个PacketSocket的实例汇聚到sink节点中


  //信道模型
  Ptr<UanPropModelIdeal> prop = CreateObjectWithAttributes<UanPropModelIdeal> ();

  /*创建uan信道*/
  NS_LOG_DEBUG ("channel set");
  Ptr<UanChannel> channel = CreateObjectWithAttributes<UanChannel> ("PropagationModel", PointerValue (prop));

  //Create net device and nodes with UanHelper

  //安装UAN信道，返回创建设备。说明设备和信道是固定匹配的
  NS_LOG_DEBUG ("device created");
  NetDeviceContainer devices = uan.Install (nc, channel);
  NetDeviceContainer sinkdev = uan.Install (sink, channel);


  /*移动模型，为每个节点分配位置信息*/
  NS_LOG_DEBUG ("Mobility Install");
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> pos = CreateObject<ListPositionAllocator> ();

 
    Ptr<UniformRandomVariable> urv = CreateObject<UniformRandomVariable> ();
    pos->Add (Vector (m_boundary / 2.0, m_boundary / 2.0, m_depth));//sink的位置
    double rsum = 0;

    double minr = 2 * m_boundary;
    for (uint32_t i = 0; i < m_numNodes; i++)
      {
        double x = urv->GetValue (0, m_boundary);
        double y = urv->GetValue (0, m_boundary);
        double newr = std::sqrt ((x - m_boundary / 2.0) * (x - m_boundary / 2.0)
                            + (y - m_boundary / 2.0) * (y - m_boundary / 2.0));
        rsum += newr;
        minr = std::min (minr, newr);
        pos->Add (Vector (x, y, m_depth));//nc的位置

      }
    NS_LOG_DEBUG ("Mean range from gateway: " << rsum / m_numNodes
                                              << "    min. range " << minr);
    
    /*安装移动模型*/   //：sink和nc节点均安装
    mobility.SetPositionAllocator (pos);//位置信息
    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");//位置信息为常量
    mobility.Install (sink);
    NS_LOG_DEBUG ("Position of sink: "
                  << sink.Get (0)->GetObject<MobilityModel> ()->GetPosition ());
    mobility.Install (nc);

    /*创建sink端的socket地址族*/  //packetSocket,是用于应用层直接对device的交互
    PacketSocketAddress socket;
    socket.SetSingleDevice (sinkdev.Get (0)->GetIfIndex ());      //设置该socket的设备,流出设备
    socket.SetPhysicalAddress (sinkdev.Get (0)->GetAddress ());   //设置目的地址,物理地址
    socket.SetProtocol (0);//协议类型？？

    /*创建OnOff应用*/
    OnOffHelper app ("ns3::PacketSocketFactory", Address (socket));//协议类型：PacketSocketFactory， socket传递目的物理地址
	
    app.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    app.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    app.SetAttribute ("DataRate", DataRateValue (m_dataRate));
    app.SetAttribute ("PacketSize", UintegerValue (m_packetSize));

    ApplicationContainer apps = app.Install (nc);//给nc节点安装应用，nc为发送端，sink为接收端，
    apps.Start (Seconds (1.5));
   apps.Stop (Seconds (1.5+40));
   
 //接收端：
    Ptr<Node> sinkNode = sink.Get (0);
    TypeId psfid = TypeId::LookupByName ("ns3::PacketSocketFactory");//socket类型，其实还可以有udp/tcp类型
    if (sinkNode->GetObject<SocketFactory> (psfid) == 0)
      {
        Ptr<PacketSocketFactory> psf = CreateObject<PacketSocketFactory> ();
        sinkNode->AggregateObject (psf);
      }
   //在sinkNode上创建socket用于接收数据包，PacketSocket类型
    Ptr<Socket> sinkSocket = Socket::CreateSocket (sinkNode, psfid);
    sinkSocket->Bind (socket);//绑定sink设备的地址
    sinkSocket->SetRecvCallback (MakeCallback (&Experiment::ReceivePacket, this));

    m_bytesTotal = 0;

    std::ofstream ascii (m_asciitracefile.c_str ());
    if (!ascii.is_open ())
      {
        NS_FATAL_ERROR ("Could not open ascii trace file: "
                        << m_asciitracefile);
      }
    uan.EnableAsciiAll (ascii);

    
    Simulator::Stop (Seconds(5+60));
    Simulator::Run ();
    /*将一些数据清零*/
    sinkNode = 0;
    sinkSocket = 0;
    pos = 0;
    channel = 0;
    prop = 0;
    for (uint32_t i=0; i < nc.GetN (); i++)
      {
        nc.Get (i) = 0;
      }
    for (uint32_t i=0; i < sink.GetN (); i++)
      {
        sink.Get (i) = 0;
      }

    for (uint32_t i=0; i < devices.GetN (); i++)
      {
        devices.Get (i) = 0;
      }
    for (uint32_t i=0; i < sinkdev.GetN (); i++)
      {
        sinkdev.Get (i) = 0;
      }
    NS_LOG_DEBUG ("End simulating ");
   Simulator::Destroy ();

   return 0;
}



int main (int argc, char **argv)
{
  //LogComponentEnable ("UanAlohaReal", LOG_LEVEL_ALL);
  LogComponentEnable ("UanMacAloha", LOG_LEVEL_ALL);
  LogComponentEnable ("UanPhyRealNew", LOG_LEVEL_ALL);
  //LogComponentEnable ("UanNetDevice", LOG_LEVEL_ALL);
  //LogComponentEnable ("PacketSocket", LOG_LEVEL_ALL);
  LogComponentEnable ("SystemThread", LOG_LEVEL_ALL);

  GlobalValue::Bind ("SimulatorImplementationType",
                     StringValue ("ns3::RealtimeSimulatorImpl"));//实时仿真，执行起来比较慢

  Experiment exp;
  std::string gnudatfile ("cwexpgnuout.dat");
  std::string perModel = "ns3::UanPhyPerGenDefault";
  std::string sinrModel = "ns3::UanPhyCalcSinrDefault";
  NS_LOG_DEBUG ("Phy set");
  /*物理层phy的配置*/
  //配置的信息存在uanhelper中,最终传递给run函数
  ObjectFactory obf;
  obf.SetTypeId (perModel);
  Ptr<UanPhyPer> per = obf.Create<UanPhyPer> ();
  obf.SetTypeId (sinrModel);
  Ptr<UanPhyCalcSinr> sinr = obf.Create<UanPhyCalcSinr> ();
  NS_LOG_DEBUG ("Tx mode set");
  UanHelper uan;  //uan助手类
  UanTxMode mode; //发送模式
  mode = UanTxModeFactory::CreateMode (UanTxMode::FSK, exp.m_dataRate,
                                       exp.m_dataRate, 12000,
                                       exp.m_dataRate, 2,
                                       "Default mode");//调制方式，datarate,symbolrate,centerfrequency,bandwidth
  UanModesList myModes;
  myModes.AppendMode (mode);

  uan.SetPhy ("ns3::UanPhyRealNew",     //"ns3::UanPhyRealNew"
              "PerModel", PointerValue (per),
              "SinrModel", PointerValue (sinr),
              "SupportedModes", UanModesListValue (myModes));//真实化时加入packetSize参数
  /*运行，调用experiment::run()*/

  exp.Run (uan);

  per = 0;
  sinr = 0;
  NS_LOG_DEBUG ("the End ");
}

