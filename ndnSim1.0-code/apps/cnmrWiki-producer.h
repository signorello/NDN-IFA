/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */ /*
 * Copyright (c) 2016 University of Luxembourg 
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
 * Author: Salvatore Signorello  <salvatore.signorello@uni.lu>
 * Some code neither licensed nor public was provided by:
 *  Hani Salah <hsalah@ke.tu-darm­stadt.de>  
 *  Julian Wulfheide <ju.wulfheide@gmail.com>
 */

#ifndef NDN_CNMRWIKI_PRODUCER_H
#define NDN_CNMRWIKI_PRODUCER_H

#include "ndn-app.h"

#include "ns3/ndnSIM/model/fw/monitor-aware-routing.h"
#include "ns3/ptr.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-data.h"

#include <vector>

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief TBD
 */
class CnmrWikiProducer : public App
{
public:
  static TypeId
  GetTypeId (void);

  CnmrWikiProducer ();

  void OnInterest (Ptr<const Interest> interest);

protected:
  virtual void
  StartApplication ();    // Called at time specified by Start

  virtual void
  StopApplication ();     // Called at time specified by Stop

  void
  SetPrefixes(std::string);

private:
  Name m_prefix;
  Name m_postfix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;

  uint32_t m_signature;
  Name m_keyLocator;

  TracedCallback<Ptr<const Interest>, bool, bool> interestConsumedTrace;
  Ptr<ndn::fw::MonitorAwareRouting> mar;

  std::vector<Name> m_prefixes;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_CNMRWIKI_PRODUCER_H
