#include "ns3/core-module.h"
 #include <unistd.h>

#include <pthread.h>
#include "ns3/system-thread.h"

#include <sys/ioctl.h>
#include <sys/types.h>

#include <netdb.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#include <linux/if_packet.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

//#include
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TestPythonNew");

int 
main (int argc, char *argv[])
{

  int sFd1 = 0, sFd2 = 0;
  int nRet = 1;

  CommandLine cmd;
  cmd.AddValue ("sFd1", "socket Fd 1", sFd1);
  cmd.AddValue ("sFd2", "socket Fd 2", sFd2);
  cmd.Parse(argc,argv);

  NS_LOG_UNCOND(sFd1);
  NS_LOG_UNCOND(sFd2);
  int tcppacketlen = 30;
  char tcpsendbuffer1[30] = "It is ns3 first test!";
  char tcpsendbuffer2[30] = "It is ns3 second test!";
 
  nRet =  send(sFd1,tcpsendbuffer1,tcppacketlen,0);
  if(nRet > 0)
     NS_LOG_UNCOND ("1send succeed");
  nRet =  send(sFd2,tcpsendbuffer2,tcppacketlen,0);
  if(nRet > 0)
     NS_LOG_UNCOND ("2send succeed");

  Simulator::Run ();
  Simulator::Destroy ();
}
