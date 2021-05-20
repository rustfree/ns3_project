#include "data-application-helper.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/string.h"
#include "ns3/data-rate.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"
#include "ns3/random-variable-stream.h"
#include "ns3/data-application.h"

namespace ns3 {

DataApplicationHelper::DataApplicationHelper (std::string protocol, Address address)
{
  m_factory.SetTypeId ("ns3::DataApplication");
  m_factory.Set ("Protocol", StringValue (protocol));
  m_factory.Set ("Remote", AddressValue (address));
}

void 
DataApplicationHelper::SetAttribute (std::string name, const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
DataApplicationHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
DataApplicationHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
DataApplicationHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
DataApplicationHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<Application> ();
  node->AddApplication (app);

  return app;
}
/*
int64_t
DataApplicationHelper::AssignStreams (NodeContainer c, int64_t stream)
{
  int64_t currentStream = stream;
  Ptr<Node> node;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      node = (*i);
      for (uint32_t j = 0; j < node->GetNApplications (); j++)
        {
          Ptr<DataApplication> Dapp = DynamicCast<DataApplication> (node->GetApplication (j));
          if (Dapp)
            {
              currentStream += Dapp->AssignStreams (currentStream);
            }
        }
    }
  return (currentStream - stream);
}
*/
void 
DataApplicationHelper::SetConstantRate (DataRate dataRate, uint32_t packetSize)
{
  
  m_factory.Set ("DataRate", DataRateValue (dataRate));
  m_factory.Set ("PacketSize", UintegerValue (packetSize));
}

//add ,from udp-echo-helper 



void
DataApplicationHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<DataApplication>()->SetFill (fill);
}

void
DataApplicationHelper::SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength)
{
  app->GetObject<DataApplication>()->SetFill (fill, dataLength);
}

void
DataApplicationHelper::SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength)
{
  app->GetObject<DataApplication>()->SetFill (fill, fillLength, dataLength);
}


} // namespace ns3
