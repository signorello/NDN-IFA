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

#include "wiki-client.h"
#include "wiki-pagetitle.h"
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
#include "ns3/ndnSIM/utils/ndn-rtt-mean-deviation.h"

#include <boost/ref.hpp>
#include <stdio.h>

#include "ns3/names.h"

NS_LOG_COMPONENT_DEFINE ("ndn.WikiClient");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (WikiClient);

TypeId
WikiClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::WikiClient")
    .SetGroupName ("Ndn")
    .SetParent<App> ()
    .AddConstructor<WikiClient> ()
    .AddAttribute ("Frequency", "Frequency of interest packets",
                   StringValue ("1.0"),
                   MakeDoubleAccessor (&WikiClient::m_frequency),
                   MakeDoubleChecker<double> ())
    
    .AddAttribute ("Randomize", "Type of send time randomization: none (default), uniform, exponential",
                   StringValue ("none"),
                   MakeStringAccessor (&WikiClient::SetRandomize, &WikiClient::GetRandomize),
                   MakeStringChecker ())
    .AddAttribute ("StartSeq", "Initial sequence number",
                   IntegerValue (0),
                   MakeIntegerAccessor(&WikiClient::m_seq),
                   MakeIntegerChecker<int32_t>())

    .AddAttribute ("Prefix","Name of the Interest",
                   StringValue ("/"),
                   MakeNameAccessor (&WikiClient::m_interestName),
                   MakeNameChecker ())
    .AddAttribute ("LifeTime", "LifeTime for interest packet",
                   StringValue ("2s"),
                   MakeTimeAccessor (&WikiClient::m_interestLifeTime),
                   MakeTimeChecker ())

    .AddAttribute ("RetxTimer",
                   "Timeout defining how frequent retransmission timeouts should be checked",
                   StringValue ("50ms"),
                   MakeTimeAccessor (&WikiClient::GetRetxTimer, &WikiClient::SetRetxTimer),
                   MakeTimeChecker ())

    .AddTraceSource ("LastRetransmittedInterestDataDelay", "Delay between last retransmitted Interest and received Data",
                     MakeTraceSourceAccessor (&WikiClient::m_lastRetransmittedInterestDataDelay))

    .AddTraceSource ("FirstInterestDataDelay", "Delay between first transmitted Interest and received Data",
                     MakeTraceSourceAccessor (&WikiClient::m_firstInterestDataDelay))
    ;

  return tid;
}

WikiClient::WikiClient ()
  : m_rand (0, std::numeric_limits<uint32_t>::max ())
  , m_random (0)
  , m_frequency (1.0)
  , m_firstTime (true)
  , m_seq (0)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_seqMax = (WikiPageTitles::Instance()->size()-1);
  m_rtt = CreateObject<RttMeanDeviation> ();
}

void
WikiClient::SetRetxTimer (Time retxTimer)
{
  m_retxTimer = retxTimer;
  if (m_retxEvent.IsRunning ())
    {
      // m_retxEvent.Cancel (); // cancel any scheduled cleanup events
      Simulator::Remove (m_retxEvent); // slower, but better for memory
    }

  // schedule even with new timeout
  m_retxEvent = Simulator::Schedule (m_retxTimer,
                                     &WikiClient::CheckRetxTimeout, this);
}

Time
WikiClient::GetRetxTimer () const
{
  return m_retxTimer;
}

void
WikiClient::CheckRetxTimeout ()
{
  Time now = Simulator::Now ();

  Time rto = m_rtt->RetransmitTimeout ();
  // NS_LOG_DEBUG ("Current RTO: " << rto.ToDouble (Time::S) << "s");

  while (!m_seqTimeouts.empty ())
    {
      SeqTimeoutsContainer::index<i_timestamp>::type::iterator entry =
        m_seqTimeouts.get<i_timestamp> ().begin ();
      if (entry->time + rto <= now) // timeout expired?
        {
          uint32_t seqNo = entry->seq;
          m_seqTimeouts.get<i_timestamp> ().erase (entry);
          OnTimeout (seqNo);
        }
      else
        break; // nothing else to do. All later packets need not be retransmitted
    }

  m_retxEvent = Simulator::Schedule (m_retxTimer,
                                     &WikiClient::CheckRetxTimeout, this);
}

// Application Methods
void
WikiClient::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS ();

  // do base stuff
  App::StartApplication ();

  ScheduleNextPacket ();
}

void
WikiClient::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS ();

  // cancel periodic packet generation
  Simulator::Cancel (m_sendEvent);

  // cleanup base stuff
  App::StopApplication ();
}

void
WikiClient::SendPacket ()
{
  if (!m_active) return;

  NS_LOG_FUNCTION_NOARGS ();

  uint32_t seq = 0; //invalid

  while (m_retxSeqs.size ())
    {
      seq = *m_retxSeqs.begin ();
      m_retxSeqs.erase (m_retxSeqs.begin ());
      break;
    }

  if (seq == 0)
    {
          if (m_seq >= m_seqMax)
            {
              return; // we are totally done
            }
      seq = m_seq++;
    }

  // Load name using the seq number as index
  std::string seqName = WikiPageTitles::Instance()->readElement(seq);
  Ptr<Name> nameWithSequence = Create<Name> (m_interestName);
  nameWithSequence->append (seqName);
  

  Ptr<Interest> interest = Create<Interest> ();
  interest->SetNonce               (m_rand.GetValue ());
  interest->SetName                (nameWithSequence);
  interest->SetInterestLifetime    (m_interestLifeTime);

  NS_LOG_INFO ("> Interest for " << seqName);

  WillSendOutInterest (seq);  

  FwHopCountTag hopCountTag;
  interest->GetPayload ()->AddPacketTag (hopCountTag);

  m_transmittedInterests (interest, this, m_face);
  m_face->ReceiveInterest (interest);

  ScheduleNextPacket ();
}

///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////


void
WikiClient::OnData (Ptr<const Data> data)
{
  if (!m_active) return;

  App::OnData (data); // tracing inside

  NS_LOG_FUNCTION (this << data);

  std::string seqName = data->GetName ().get (-1).toBlob ();
  uint32_t seq = WikiPageTitles::Instance()->find(seqName);
  NS_LOG_INFO ("< DATA for " << seqName);

  int hopCount = -1;
  FwHopCountTag hopCountTag;
  if (data->GetPayload ()->PeekPacketTag (hopCountTag))
    {
      hopCount = hopCountTag.Get ();
    }

  SeqTimeoutsContainer::iterator entry = m_seqLastDelay.find (seq);
  if (entry != m_seqLastDelay.end ())
    {
      m_lastRetransmittedInterestDataDelay (this, seq, Simulator::Now () - entry->time, hopCount);
    }

  entry = m_seqFullDelay.find (seq);
  if (entry != m_seqFullDelay.end ())
    {
      m_firstInterestDataDelay (this, seq, Simulator::Now () - entry->time, m_seqRetxCounts[seq], hopCount);
    }

  m_seqRetxCounts.erase (seq);
  m_seqFullDelay.erase (seq);
  m_seqLastDelay.erase (seq);

  m_seqTimeouts.erase (seq);
  m_retxSeqs.erase (seq);

  m_rtt->AckSeq (SequenceNumber32 (seq));
}

void
WikiClient::OnNack (Ptr<const Interest> interest)
{
  if (!m_active) return;

  App::OnNack (interest); // tracing inside

  std::string seqName = interest->GetName ().get (-1).toBlob ();
  uint32_t seq = WikiPageTitles::Instance()->find(seqName);

  NS_LOG_INFO ("< NACK for " << seqName);

  m_retxSeqs.insert (seq);

  m_seqTimeouts.erase (seq);

  m_rtt->IncreaseMultiplier ();             // Double the next RTO ??
  ScheduleNextPacket ();
}

void
WikiClient::ScheduleNextPacket ()
{
  if (m_firstTime)
    {
      m_sendEvent = Simulator::Schedule (Seconds (0.0),
                                         &WikiClient::SendPacket, this);
      m_firstTime = false;
    }
  else if (!m_sendEvent.IsRunning ())
    m_sendEvent = Simulator::Schedule (
                                       (m_random == 0) ?
                                         Seconds(1.0 / m_frequency)
                                       :
                                         Seconds(m_random->GetValue ()),
                                       &WikiClient::SendPacket, this);
}

void
WikiClient::OnTimeout (uint32_t sequenceNumber)
{
  m_rtt->IncreaseMultiplier ();             // Double the next RTO
  m_rtt->SentSeq (SequenceNumber32 (sequenceNumber), 1); // make sure to disable RTT calculation for this sample
  m_retxSeqs.insert (sequenceNumber);
  ScheduleNextPacket ();
}

void
WikiClient::WillSendOutInterest (uint32_t sequenceNumber)
{
  NS_LOG_DEBUG ("Trying to add " << sequenceNumber << " with " << Simulator::Now () << ". already " << m_seqTimeouts.size () << " items");

  m_seqTimeouts.insert (SeqTimeout (sequenceNumber, Simulator::Now ()));
  m_seqFullDelay.insert (SeqTimeout (sequenceNumber, Simulator::Now ()));

  m_seqLastDelay.erase (sequenceNumber);
  m_seqLastDelay.insert (SeqTimeout (sequenceNumber, Simulator::Now ()));

  m_seqRetxCounts[sequenceNumber] ++;

  m_rtt->SentSeq (SequenceNumber32 (sequenceNumber), 1);
}

void
WikiClient::SetRandomize (const std::string &value)
{
  if (m_random)
    delete m_random;

  if (value == "uniform")
    {
      m_random = new UniformVariable (0.0, 2 * 1.0 / m_frequency);
    }
  else if (value == "exponential")
    {
      m_random = new ExponentialVariable (1.0 / m_frequency, 50 * 1.0 / m_frequency);
    }
  else
    m_random = 0;

  m_randomType = value;  
}

std::string
WikiClient::GetRandomize () const
{
  return m_randomType;
}

} // namespace ndn
} // namespace ns3
