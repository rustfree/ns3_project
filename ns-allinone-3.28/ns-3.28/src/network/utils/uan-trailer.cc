/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Emmanuelle Laprise <emmanuelle.laprise@bluekazoo.ca>
 */

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/trailer.h"
#include "uan-trailer.h"
#include "crc32.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("UanTrailer");

NS_OBJECT_ENSURE_REGISTERED (UanTrailer);

UanTrailer::UanTrailer ()
  : m_calcFcs (false),
    m_fcs (0)
{
  NS_LOG_FUNCTION (this);
}

void
UanTrailer::EnableFcs (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_calcFcs = enable;
}

bool
UanTrailer::CheckFcs (Ptr<const Packet> p) const
{
  NS_LOG_FUNCTION (this << p);
  int len = p->GetSize ();
  uint8_t *buffer;
  uint32_t crc;

  if (!m_calcFcs)
    {
      return true;
    }

  buffer = new uint8_t[len];
  p->CopyData (buffer, len);
  crc = CRC32Calculate (buffer, len);
  delete[] buffer;
  return (m_fcs == crc);
}

void
UanTrailer::CalcFcs (Ptr<const Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  int len = p->GetSize ();
  uint8_t *buffer;

  if (!m_calcFcs)
    {
      return;
    }

  buffer = new uint8_t[len];
  p->CopyData (buffer, len);
  m_fcs = CRC32Calculate (buffer, len);
  delete[] buffer;
}

void
UanTrailer::SetFcs (uint32_t fcs)
{
  NS_LOG_FUNCTION (this << fcs);
  m_fcs = fcs;
}

uint32_t
UanTrailer::GetFcs (void)
{
  NS_LOG_FUNCTION (this);
  return m_fcs;
}

uint32_t
UanTrailer::GetTrailerSize (void) const
{
  NS_LOG_FUNCTION (this);
  return GetSerializedSize ();
}

TypeId 
UanTrailer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UanTrailer")
    .SetParent<Trailer> ()
    .SetGroupName("Network")
    .AddConstructor<UanTrailer> ()
  ;
  return tid;
}
TypeId 
UanTrailer::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
void 
UanTrailer::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "fcs=" << m_fcs;
}
uint32_t 
UanTrailer::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return 4;
}

void
UanTrailer::Serialize (Buffer::Iterator end) const
{
  NS_LOG_FUNCTION (this << &end);
  Buffer::Iterator i = end;
  i.Prev (GetSerializedSize ());

  i.WriteU32 (m_fcs);
}
uint32_t
UanTrailer::Deserialize (Buffer::Iterator end)
{
  NS_LOG_FUNCTION (this << &end);
  Buffer::Iterator i = end;
  uint32_t size = GetSerializedSize ();
  i.Prev (size);

  m_fcs = i.ReadU32 ();

  return size;
}

} // namespace ns3