/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*

 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef DATA_APPLICATION_HELPER_H
#define DATA_APPLICATION_HELPER_H

#include <stdint.h>
#include <string>
#include "ns3/object-factory.h"
#include "ns3/address.h"
#include "ns3/attribute.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"
#include "ns3/data-application.h"

namespace ns3 {

class DataRate;

/**
 * \ingroup onoff
 * \brief A helper to make it easier to instantiate an ns3::OnOffApplication 
 * on a set of nodes.
 */
class DataApplicationHelper
{
public:
 
  DataApplicationHelper (std::string protocol, Address address);

  
  void SetAttribute (std::string name, const AttributeValue &value);

  void SetConstantRate (DataRate dataRate, uint32_t packetSize = 512);

  ApplicationContainer Install (NodeContainer c) const;

  ApplicationContainer Install (Ptr<Node> node) const;


  ApplicationContainer Install (std::string nodeName) const;

  int64_t AssignStreams (NodeContainer c, int64_t stream);
  
  void SetFill (Ptr<Application> app, std::string fill);
  void SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength);
  void SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength);

private:

  Ptr<Application> InstallPriv (Ptr<Node> node) const;

  ObjectFactory m_factory; //!< Object factory.
};

} // namespace ns3

#endif /* ON_OFF_HELPER_H */

