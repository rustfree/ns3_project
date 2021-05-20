/*
*  uan-aloha-example.h
*  Created on: 2018/4/11
*   Author:lch
*
*/


#ifndef UAN_ALOHA_REAL_EXAMPLE_H
#define UAN_ALOHA_REAL_EXAMPLE_H

#include "ns3/network-module.h"
#include "ns3/stats-module.h"
#include "ns3/uan-module.h"

using namespace ns3;

class Experiment
{
public:

int  Run (UanHelper &uan);
 
  void ReceivePacket (Ptr<Socket> socket);

  //void UpdatePositions (NodeContainer &nodes);

  void ResetData ();


  
  uint32_t m_numNodes;                //!< Number of transmitting nodes.
  uint32_t m_dataRate;                //!< DataRate in bps.
  double m_depth;                     //!< Depth of transmitting and sink nodes.
  double m_boundary;                  //!< Size of boundary in meters.
  uint32_t m_packetSize;              //!< Generated packet size in bytes.
  uint32_t m_bytesTotal;              //!< Total bytes received.

  Time m_simTime;                     //!< Simulation run time, default 1000 s.

  std::string m_gnudatfile;           //!< Name for GNU Plot output, default uan-cw-example.gpl.
  std::string m_asciitracefile;       //!< Name for ascii trace file, default uan-cw-example.asc.
  std::string m_bhCfgFile;            //!< (Unused)

  Gnuplot2dDataset m_data;            //!< Container for the simulation data.
  std::vector<double> m_throughputs;  //!< Throughput for each run.

  /** Default constructor. */
  Experiment ();
};

#endif /* UAN_ALOHA_EXAMPLE_H */
