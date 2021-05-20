/*   ~ * ~ * ~ * ~ * ~ * ~ * ~ * ~ * ~ * ~ * 
 **    data-application.cc                     *
 **    Created on 2018/5/7                *
 **    Author:lch                          *
 **    For:send packet with real data    *
 **  ~ * ~ * ~ * ~ * ~ * ~ * ~ * ~ * ~ * ~ * 
*/

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/trace-source-accessor.h"
#include "data-application.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DataApplication");

NS_OBJECT_ENSURE_REGISTERED (DataApplication);

TypeId
DataApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DataApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<DataApplication> ()
    .AddAttribute ("PacketSize", "Size of echo data in outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&DataApplication::SetDataSize,&DataApplication::GetDataSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Mean", 
                   "The mean of ExponentialRandomVariable",
                   DoubleValue (10),
                   MakeDoubleAccessor (&DataApplication::m_mean),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Bound", 
                   "The Bound of ExponentialRandomVariable",
                   DoubleValue (20),
                   MakeDoubleAccessor (&DataApplication::m_bound),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Remote", "The address of the destination",
                   AddressValue (),
                   MakeAddressAccessor (&DataApplication::m_peer),
                   MakeAddressChecker ())
    .AddAttribute ("Protocol", "The type of protocol to use.",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&DataApplication::m_tid),
                   MakeTypeIdChecker ())
   /* .AddAttribute ("PtrRandom", "The pointer of Random variable.",
                   PointerValue (),
                   MakePointerAccessor (&DataApplication::m_erv),
                   MakePointerChecker<ExponentialRandomVariable> ())*/
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&DataApplication::m_txTrace),
                     "ns3::Packet::TracedCallback")
  ;
  return tid;
}

DataApplication::DataApplication ()
  : m_socket (0),
    m_connected (false),
    m_residualBits (0),//?????
    m_lastStartTime (Seconds (0)),
    m_totBytes (0)
{
  NS_LOG_FUNCTION (this);
}

DataApplication::~DataApplication()
{
  NS_LOG_FUNCTION (this);
}

Ptr<Socket>
DataApplication::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

void
DataApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  // chain up
  Application::DoDispose ();
}

// Application Methods
void DataApplication::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  NodeId = GetNode()->GetId();
  // Create the socket if not already
  if (!m_socket)
    {
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      if (Inet6SocketAddress::IsMatchingType (m_peer))
        {
          m_socket->Bind6 ();
        }
      else if (InetSocketAddress::IsMatchingType (m_peer) ||
               PacketSocketAddress::IsMatchingType (m_peer))
        {
          m_socket->Bind ();
        }
      m_socket->Connect (m_peer);
      m_socket->SetAllowBroadcast (true);
      m_socket->ShutdownRecv ();

      m_socket->SetConnectCallback (
      MakeCallback (&DataApplication::ConnectionSucceeded, this),
      MakeCallback (&DataApplication::ConnectionFailed, this));
    }
  
  m_erv = CreateObject<ExponentialRandomVariable> ();
  //RngSeedManager::SetSeed(3);
  // RngSeedManager::SetRun(5);
  m_socket->SetRecvCallback (MakeCallback (&DataApplication::HandleRead, this));
  if(m_size > 0)
  	ScheduleTransmit (Seconds (0.));
}


void 
DataApplication::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  
  NS_LOG_UNCOND("NodeId: "<<NodeId<<"--DataApplication-- txPktNum:"<<txPktNum);
  if (m_socket != 0) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
      m_socket = 0;
    }

  Simulator::Cancel (m_sendEvent);
}



void 
DataApplication::SetDataSize (uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << dataSize);

  //
  // If the client is setting the echo packet data size this way, we infer
  // that she doesn't care about the contents of the packet at all, so 
  // neither will we.
  //
 

  //delete [] m_data;
  m_data = 0;
  m_dataSize = 0;
  m_size = dataSize;
  

}

uint32_t 
DataApplication::GetDataSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_size;
}

void 
DataApplication::SetFill (std::string fill)
{
  NS_LOG_FUNCTION (this << fill);

  uint32_t dataSize = fill.size () + 1;

  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memcpy (m_data, fill.c_str (), dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void 
DataApplication::SetFill (uint8_t fill, uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << fill << dataSize);
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  memset (m_data, fill, dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void 
DataApplication::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << fill << fillSize << dataSize);
  if (dataSize != m_dataSize)
    {
      delete [] m_data;
      m_data = new uint8_t [dataSize];
      m_dataSize = dataSize;
    }

  if (fillSize >= dataSize)
    {
      memcpy (m_data, fill, dataSize);
      m_size = dataSize;
      return;
    }

  //
  // Do all but the final fill.
  //
  uint32_t filled = 0;
  while (filled + fillSize < dataSize)
    {
      memcpy (&m_data[filled], fill, fillSize);
      filled += fillSize;
    }

  //
  // Last fill may be partial
  //
  memcpy (&m_data[filled], fill, dataSize - filled);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}


void 
DataApplication::ScheduleTransmit (Time dt)
{
  NS_LOG_FUNCTION (this << dt);
  double interval = 0;
  interval = m_erv->GetValue (m_mean, m_bound);
/*
  while(interval <= 1.0){
    interval = m_erv->GetValue (m_mean, m_bound);
     } */
  m_time = interval;
  //NS_LOG_UNCOND("nodeId:"<<NodeId<< "-Random time:"<<m_time);
  m_sendEvent = Simulator::Schedule (Seconds(m_time), &DataApplication::Send, this);
}

void 
DataApplication::Send (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());
  Ptr<Packet> p;
  if (m_dataSize)
    {
      p = Create<Packet> (m_data, m_dataSize);//m_data??
    }
  else
    {
      p = Create<Packet> (m_size);
    }
  m_txTrace (p);
  m_socket->Send (p);

  //++m_sent; 
  txPktNum ++;
  
  ScheduleTransmit (Seconds(0));

}

void
DataApplication::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s packet sink received "
                       <<  packet->GetSize () << " bytes ");
    }
}
void DataApplication::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  m_connected = true;
}

void DataApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

} // Namespace ns3
