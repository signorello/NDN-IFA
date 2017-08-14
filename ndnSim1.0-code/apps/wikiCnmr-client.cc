/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
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



#include "wiki-client.h"
#include "wiki-pagetitle.h"
#include "wikiCnmr-client.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"

#include "ns3/ndn-app-face.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ns3/ndnSIM/utils/interest-type-tag.h"
#include "ns3/ndnSIM/utils/ndn-rtt-mean-deviation.h"

#include <boost/ref.hpp>
#include <stdio.h>

#include <boost/algorithm/string.hpp>

#include "ns3/names.h"
#include "ns3/abort.h"

NS_LOG_COMPONENT_DEFINE ("ndn.WikiCnmrClient");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (WikiCnmrClient);

TypeId
WikiCnmrClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::WikiCnmrClient")
    .SetGroupName ("Ndn")
    .SetParent<WikiClient> ()
    .AddConstructor<WikiCnmrClient> ()
    .AddAttribute ("Prefixes", "Comma-seperated string of prefixes this client can request",
                   StringValue ("google.com,"),
                   MakeStringAccessor (&WikiCnmrClient::SetPrefixes),
                   MakeStringChecker ())
    .AddAttribute ("q", "parameter of improve rank",
                   StringValue ("0.7"),
                   MakeDoubleAccessor (&WikiCnmrClient::SetQ, &WikiCnmrClient::GetQ),
                   MakeDoubleChecker<double> ())

    .AddAttribute ("s", "parameter of power",
                   StringValue ("0.7"),
                   MakeDoubleAccessor (&WikiCnmrClient::SetS, &WikiCnmrClient::GetS),
                   MakeDoubleChecker<double> ())

    .AddAttribute ("NumberOfContents", "Number of the Contents in total",
                   StringValue ("1024"),
                   MakeUintegerAccessor (&WikiCnmrClient::SetNumberOfContents, &WikiCnmrClient::GetNumberOfContents),
                   MakeUintegerChecker<uint32_t> ())

    .AddAttribute ("StartingIndex", "Starting index to navigate the wikipedia bucket",
                   StringValue ("0"),
                   MakeUintegerAccessor (&WikiCnmrClient::SetStartingIndex),
                   MakeUintegerChecker<uint32_t> ())
    ;

  return tid;
}

WikiCnmrClient::WikiCnmrClient ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

// Application Methods
void
WikiCnmrClient::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS ();

  mar = GetNode()->GetObject<ndn::fw::MonitorAwareRouting>();
  if (mar != 0)
    mar->setHasClient();

  uint32_t wikiFileSize = WikiPageTitles::Instance()->size();
  NS_ABORT_MSG_IF(wikiFileSize == 0, "The list of content names results empty, check why it has not been loaded properly");
  // The following is commented out since the arguments passed to the setNumberOfContents is used to
  // instantiate an array of doubles. If we pass the whole bucket size, we'll get a std:alloc error.
  // SetNumberOfContents (wikiFileSize);

  App::StartApplication ();

  ScheduleNextPacket ();
}

void
WikiCnmrClient::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS ();

  WikiClient::StopApplication ();
}

void WikiCnmrClient::SetPrefixes(std::string strPrefixes)
{
    std::vector<std::string> strs;
    boost::split(strs, strPrefixes, boost::is_any_of(","));
    prefixes.clear();

    for(size_t i = 0; i < strs.size(); i++)
    {
        prefixes.push_back(Name(strs[i]));
    }

    rngPrefix = new UniformVariable (0, strs.size());
}

void WikiCnmrClient::SetStartingIndex(uint32_t offset)
{
  m_index_offset = offset;
}

Name &WikiCnmrClient::GetNextPrefix()
{
    return prefixes[rngPrefix->GetInteger()];
}

void
WikiCnmrClient::SendPacket ()
{
  if (!m_active) return;

  NS_LOG_FUNCTION_NOARGS ();

  uint32_t seq = GetNextSeq();

  // The index is computed somewhere else where clients can be distinguished, so in the following
  // call we just use it 
  
  NS_LOG_INFO ("> Index for the bucket is " << (m_index_offset + seq));
  std::string seqName = WikiPageTitles::Instance()->readElement(m_index_offset + seq);
  Ptr<Name> nameWithSequence = Create<Name> (GetNextPrefix());
  nameWithSequence->append (seqName);
  //

  Ptr<Interest> interest = Create<Interest> ();
  interest->SetNonce               (m_rand.GetValue ());
  interest->SetName                (nameWithSequence);
  interest->SetInterestLifetime    (m_interestLifeTime);

  NS_LOG_INFO ("> Interest for " << seqName);

  WillSendOutInterest (seq);  

  FwHopCountTag hopCountTag;
  hopCountTag.Add(GetNode()->GetId());
  interest->GetPayload ()->AddPacketTag (hopCountTag);

  InterestTypeTag interestTypeTag;
  interestTypeTag.Set(1);
  interest->GetPayload ()->AddPacketTag(interestTypeTag);

  m_transmittedInterests (interest, this, m_face);
  m_face->ReceiveInterest (interest);

  ScheduleNextPacket ();
}

void
WikiCnmrClient::ScheduleNextPacket ()
{
  
  if (m_firstTime)
    {
      m_sendEvent = Simulator::Schedule (Seconds (0.0),
                                         &WikiCnmrClient::SendPacket, this);
      m_firstTime = false;
    }
  else if (!m_sendEvent.IsRunning ())
    m_sendEvent = Simulator::Schedule (
                                       (m_random == 0) ?
                                         Seconds(1.0 / m_frequency)
                                       :
                                         Seconds(m_random->GetValue ()),
                                       &WikiCnmrClient::SendPacket, this);
}

uint32_t WikiCnmrClient::GetNextSeq()
{
    uint32_t content_index = 1; //[1, m_N]
    double p_sum = 0;

    double p_random = m_SeqRng.GetValue();
    while (p_random == 0)
    {
        p_random = m_SeqRng.GetValue();
    }

    for (uint32_t i=1; i<=m_N; i++)
    {
        p_sum = m_Pcum[i];   //m_Pcum[i] = m_Pcum[i-1] + p[i], p[0] = 0;   e.g.: p_cum[1] = p[1], p_cum[2] = p[1] + p[2]
        if (p_random <= p_sum)
        {
            content_index = i;
            break;
        }
    }
    return content_index;
}

void WikiCnmrClient::SetNumberOfContents (uint32_t numOfContents)
{
    m_N = numOfContents - 1;

    m_Pcum = std::vector<double> (m_N + 1);

    m_Pcum[0] = 0.0;
    for (uint32_t i=1; i<=m_N; i++)
    {
        m_Pcum[i] = m_Pcum[i-1] + 1.0 / std::pow(i+m_q, m_s);
    }

    for (uint32_t i=1; i<=m_N; i++)
    {
        m_Pcum[i] = m_Pcum[i] / m_Pcum[m_N];
    }
}

uint32_t WikiCnmrClient::GetNumberOfContents () const
{
    return m_N;
}

void WikiCnmrClient::SetQ (double q)
{
    m_q = q;
}

double WikiCnmrClient::GetQ () const
{
    return m_q;
}

void WikiCnmrClient::SetS (double s)
{
    m_s = s;
}

double WikiCnmrClient::GetS () const
{
    return m_s;
}

} // namespace ndn
} // namespace ns3
