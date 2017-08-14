/*
Copyright (C) 2016, the University of Luxembourg
Salvatore Signorello <salvatore.signorello@uni.lu>

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This software is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef NDN_WIKI_PRODUCER_H
#define NDN_WIKI_PRODUCER_H

#include "ndn-app.h"

#include "ns3/ptr.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-data.h"

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief This is a slight enhancement of the simple Producer app
 *
 * A simple Interest-sink application which replies every incoming Interest with Data
 * packet with a specified size and name same as in Interest as soon as the Interest
 * name exists in the list held by the Singleton class WikiPageTitles.
 */
class WikiProducer : public App
{
public:
  static TypeId
  GetTypeId (void);

  WikiProducer ();

  void OnInterest (Ptr<const Interest> interest);

protected:
  // inherited from Application base class.
  virtual void
  StartApplication ();

  virtual void
  StopApplication ();

private:
  Name m_prefix;
  Name m_postfix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;

  uint32_t m_signature;
  Name m_keyLocator;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_WIKI_PRODUCER_H
