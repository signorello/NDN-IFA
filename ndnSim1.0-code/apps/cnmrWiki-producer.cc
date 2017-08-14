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

#include "wiki-pagetitle.h"
#include "cnmrWiki-producer.h"
#include "ns3/log.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include "ns3/ndn-app-face.h"
#include "ns3/ndn-fib.h"

#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include <boost/algorithm/string.hpp>

#include <boost/ref.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace ll = boost::lambda;

NS_LOG_COMPONENT_DEFINE ("ndn.CnmrWikiProducer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (CnmrWikiProducer);

TypeId
CnmrWikiProducer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::CnmrWikiProducer")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<CnmrWikiProducer> ()
    .AddTraceSource ("InterestConsumed",  "InterestConsumed",  MakeTraceSourceAccessor (&CnmrWikiProducer::interestConsumedTrace))
    .AddAttribute ("Prefix","Prefix, for which CnmrWikiProducer has the data",
                   StringValue ("/"),
                   MakeStringAccessor (&CnmrWikiProducer::SetPrefixes),
                   MakeStringChecker ())
    .AddAttribute ("Postfix", "Postfix that is added to the output data (e.g., for adding CnmrWikiProducer-uniqueness)",
                   StringValue ("/"),
                   MakeNameAccessor (&CnmrWikiProducer::m_postfix),
                   MakeNameChecker ())
    .AddAttribute ("PayloadSize", "Virtual payload size for Content packets",
                   UintegerValue (1024),
                   MakeUintegerAccessor (&CnmrWikiProducer::m_virtualPayloadSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                   TimeValue (Seconds (0)),
                   MakeTimeAccessor (&CnmrWikiProducer::m_freshness),
                   MakeTimeChecker ())
    .AddAttribute ("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                   UintegerValue (0),
                   MakeUintegerAccessor (&CnmrWikiProducer::m_signature),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                   NameValue (),
                   MakeNameAccessor (&CnmrWikiProducer::m_keyLocator),
                   MakeNameChecker ())
    ;
  return tid;
}

CnmrWikiProducer::CnmrWikiProducer ()
{
  // NS_LOG_FUNCTION_NOARGS ();
}

void CnmrWikiProducer::SetPrefixes(std::string strPrefixes)
{
    std::vector<std::string> strs;
    boost::split(strs, strPrefixes, boost::is_any_of(","));
    m_prefixes.clear();

    for(size_t i = 0; i < strs.size(); i++)
    {
        m_prefixes.push_back(Name(strs[i]));
    }
}

// inherited from Application base class.
void
CnmrWikiProducer::StartApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);

  App::StartApplication ();

  mar = GetNode()->GetObject<ndn::fw::MonitorAwareRouting>();
  if (mar != 0)
    mar->setHasServer();

  NS_LOG_DEBUG ("NodeID: " << GetNode ()->GetId ());

  Ptr<Fib> fib = GetNode ()->GetObject<Fib> ();

  std::vector<Name>::iterator it;
  for ( it = m_prefixes.begin(); it != m_prefixes.end(); it++){

    Ptr<fib::Entry> fibEntry = fib->Add (*it, m_face, 0);

    fibEntry->UpdateStatus (m_face, fib::FaceMetric::NDN_FIB_GREEN);
  }

}

void
CnmrWikiProducer::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  NS_ASSERT (GetNode ()->GetObject<Fib> () != 0);

  App::StopApplication ();
}


void
CnmrWikiProducer::OnInterest (Ptr<const Interest> interest)
{
  App::OnInterest (interest); // tracing inside

  NS_LOG_FUNCTION (this << interest);

  if (!m_active) return;


  bool legitimateRequest = true;
  std::string seqName = interest->GetName ().get (-1).toBlob ();
  NS_LOG_INFO("Received Interest for "<< seqName);
  if(WikiPageTitles::Instance()->find(seqName) == -1)
    legitimateRequest = false;

  FwHopCountTag hopCountTag;
  interest->GetPayload ()->PeekPacketTag (hopCountTag);

  if(legitimateRequest)
  {
    Ptr<Data> data = Create<Data> (Create<Packet> (m_virtualPayloadSize));
    Ptr<Name> dataName = Create<Name> (interest->GetName ());
    dataName->append (m_postfix);
    data->SetName (dataName);
    data->SetFreshness (m_freshness);
    data->SetTimestamp (Simulator::Now());

    data->SetSignature (m_signature);
    if (m_keyLocator.size () > 0)
      {
        data->SetKeyLocator (Create<Name> (m_keyLocator));
      }

    NS_LOG_INFO ("Responding with Data: " << data->GetName ());

    // Echo back FwHopCountTag if exists
    FwHopCountTag hopCountTag;
    if (interest->GetPayload ()->PeekPacketTag (hopCountTag))
      {
        data->GetPayload ()->AddPacketTag (hopCountTag);
      }

    m_face->ReceiveData (data);
    m_transmittedDatas (data, this, m_face);
  }
  else
    {
        NS_LOG_INFO("Cannot respond to interest for " << interest->GetName());
    }

    // (Possible) forward the interest if it has not yet been monitored. The interest is only
    // forwarded to another if if FTBM is enabled. The MonitorAwareRouting takes care of this.
    if(interest->GetMonitored() == 0 && mar != 0)
    {
        NS_LOG_DEBUG("Served interest but it's not monitored. Forwarding...");

        Ptr<Name> nameWithSequence = Create<Name> (interest->GetName ());

        Ptr<Interest> newInterest = Create<Interest> ();
        newInterest->SetNonce (interest->GetNonce ());
        newInterest->SetName (interest->GetName ());
        newInterest->SetInterestLifetime (interest->GetInterestLifetime ());

        if(legitimateRequest)
        {
            newInterest->SetServed(1);
        }
	else
	    newInterest->SetServed(2);

        newInterest->GetPayload()->AddPacketTag(hopCountTag);

        m_face->ReceiveInterest (newInterest);
    }
    else
    {
        // If the interest has been monitored before, and served by this node, it is not forwarded
        // any further -> print the hops (but don't add self to hop list becase the interest leaves
        // the AS at this point).
        interestConsumedTrace(interest, false, legitimateRequest);
    }
}

} // namespace ndn
} // namespace ns3
