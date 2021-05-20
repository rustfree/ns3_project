/*   ~ * ~ * ~ * ~ * ~ * ~ * ~ * ~ * ~ * ~ * 
 **    data-application.h                     *
 **    Created on 2018/5/7                *
 **    Author:lch                          *
 **    For:send packet with real data    *
 **  ~ * ~ * ~ * ~ * ~ * ~ * ~ * ~ * ~ * ~ * 
*/

#ifndef DATA_APPLICATION_H
#define DATA_APPLICATION_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include <fcntl.h> 
#include <stdlib.h> 
#include "ns3/random-variable-stream.h"


namespace ns3 {

class Address;
class RandomVariableStream;
class Socket;

class DataApplication : public Application 
{
public:


static TypeId GetTypeId (void);

  DataApplication ();

virtual ~DataApplication();



  Ptr<Socket> GetSocket (void) const;
  void SetRemote (Address ip, uint16_t port);
  /**
   * \brief set the remote address
   * \param addr remote address
   */
  void SetRemote (Address addr);

  void SetDataSize (uint32_t dataSize);
  uint32_t GetDataSize (void) const;
  void SetFill (std::string fill);
  void SetFill (uint8_t fill, uint32_t dataSize); 
  void SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize);
  void HandleRead (Ptr<Socket> socket);

protected:
  virtual void DoDispose (void);

private:
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop


  void ScheduleTransmit (Time dt);
  void Send (void);
  //onoff
  void ConnectionSucceeded (Ptr<Socket> socket);
  void ConnectionFailed (Ptr<Socket> socket);


  uint32_t m_dataSize; //!< packet payload size (must be equal to m_size)
  uint8_t *m_data;
  FILE *fp;
  uint16_t NodeId;

  double m_time = 0;
  //指数随机变量参数
  double m_mean = 10;
  double m_bound = 20;
  Ptr<ExponentialRandomVariable> m_erv;

  uint32_t m_size; //!< Size of the sent packet


  uint32_t txPktNum = 0; //!< Counter for sent packets
  Ptr<Socket> m_socket; //!< Socket
  Address m_peerAddress; //!< Remote peer address
  uint16_t m_peerPort; //!< Remote peer port


  /// Callbacks for tracing the packet Tx events
  TracedCallback<Ptr<const Packet> > m_txTrace;
 
  Address         m_peer;         //!< Peer address
  bool            m_connected;    //!< True if connected


  uint32_t        m_residualBits; //!< Number of generated, but not sent, bits
  Time            m_lastStartTime; //!< Time last packet sent
  uint64_t        m_maxBytes;     //!< Limit total number of bytes sent
  uint64_t        m_totBytes;     //!< Total bytes sent so far
  EventId         m_startStopEvent;     //!< Event id for next start or stop event
  EventId         m_sendEvent;    //!< Event id of pending "send packet" event
  TypeId          m_tid;          //!< Type of the socket used


};

} // namespace ns3

#endif /* ONOFF_APPLICATION_H */
