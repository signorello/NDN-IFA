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
 *  Hani Salah <hsalah@ke.tu-darmstadt.de>  
 *  Julian Wulfheide <ju.wulfheide@gmail.com>
 */

#include "ndn-consumer.h"
#include "cnmrWikiAttacker.h"
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

#include "ns3/ndn-l3-protocol.h"
#include "ns3/ndn-app-face.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndn-pit.h"

#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"

#include <fstream>
#include <iostream>
#include <math.h>

NS_LOG_COMPONENT_DEFINE ("ndn.CnmrWikiAttacker");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (CnmrWikiAttacker);

TypeId CnmrWikiAttacker::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::ndn::CnmrWikiAttacker")
        .SetGroupName ("Ndn")
        .SetParent<App> ()
        .AddConstructor<CnmrWikiAttacker> ()

        .AddAttribute ("Frequency", "Frequency of interest packets",
                StringValue ("1.0"),
                MakeDoubleAccessor (&CnmrWikiAttacker::m_frequency),
                MakeDoubleChecker<double> ())

        .AddAttribute ("Prefix","Name of the Interest",
                StringValue ("/"),
                MakeNameAccessor (&CnmrWikiAttacker::m_prefixName),
                MakeNameChecker ())

        .AddAttribute ("LifeTime", "LifeTime for interest packet",
                StringValue ("2s"),
                MakeTimeAccessor (&CnmrWikiAttacker::m_interestLifeTime),
                MakeTimeChecker ())

        .AddAttribute ("StartAt",
                "When to start the attack. Attackers do nothing before.",
                TimeValue (Minutes (10)),
                MakeTimeAccessor (&CnmrWikiAttacker::m_startAt),
                MakeTimeChecker ())

        .AddAttribute ("StopAt",
                "When to stop the attack. Attackers do nothing after.",
                TimeValue (Minutes (20)),
                MakeTimeAccessor (&CnmrWikiAttacker::m_stopAt),
                MakeTimeChecker ())

        .AddAttribute ("fakeList",
                "Absolute path to the file containing fake suffixes",
                StringValue ("./suffix.txt"),
                MakeStringAccessor (&CnmrWikiAttacker::m_fakeList),
                ns3::MakeStringChecker ())
	.AddAttribute ("purity",
                "This value represents the percentage of existent contents requested each second",
                StringValue ("1.0"),
                MakeDoubleAccessor (&CnmrWikiAttacker::m_purity),
                MakeDoubleChecker<double> ())
	
        ;
    return tid;
}

CnmrWikiAttacker::CnmrWikiAttacker()
  : m_firstTime (true){}

void CnmrWikiAttacker::loadFakePrefixesList()
{
  uint32_t titles_counter = 0;
  std::string current_title = "";
  std::ifstream titles_file;
  titles_file.open(m_fakeList.c_str());
  NS_LOG_INFO ("Loading file " << m_fakeList);
  while(getline(titles_file,current_title))
  {
    m_fakeSuffixes.push_back(current_title);
    titles_counter++; 
  }
  titles_file.close();

  m_sizeFakeList = m_fakeSuffixes.size();
  NS_ASSERT_MSG (m_sizeFakeList != 0, "The attacker app is not getting any list of contents!");
  NS_LOG_INFO ("Fake list loaded with " << titles_counter << " titles");
  NS_LOG_INFO ("The bucket has size " << m_sizeFakeList << " titles"); 
}

std::string CnmrWikiAttacker::readFakeElement(uint32_t index)
{
  return m_fakeSuffixes[index % m_sizeFakeList];
}

std::string CnmrWikiAttacker::readTrueElement(uint32_t index)
{
  return WikiPageTitles::Instance()->readElement(index);;
}

void CnmrWikiAttacker::StartApplication()
{
    NS_LOG_FUNCTION_NOARGS ();

    NS_ASSERT_MSG (m_purity <= 1.0, "The purity value must be less than or equal to 1.0");

    loadFakePrefixesList();

    App::StartApplication();

    m_face->SetFlags(FLAG); // this is used by the MR nodes to detect specific faces

    m_randNonce = UniformVariable (0, std::numeric_limits<uint32_t>::max ());
    m_randomSeqId = UniformVariable (1, std::numeric_limits<uint32_t>::max ());
    m_randomTime = UniformVariable (0.0, 2 * 1.0 / m_frequency);

    if (m_purity != 1.0)
      Simulator::Schedule (m_startAt, &CnmrWikiAttacker::ResetCounters, this);
    else
      NS_LOG_INFO("This attacker will only generate Interests for non-existing contents");

    Simulator::Schedule (m_startAt, &CnmrWikiAttacker::ScheduleNextPacket, this);

}

void CnmrWikiAttacker::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();

  // cancel last packet generation event
  Simulator::Cancel (m_sendEvent);

  // cleanup base stuff
  App::StopApplication ();
}

void CnmrWikiAttacker::ResetCounters()
{
  if (!m_firstTime){
    NS_LOG_INFO("Good Interests "<< goodIs << " Bad Interests "<< badIs);
    goodIs = 0;
    badIs = 0 ;
  }
  else
    NS_LOG_INFO("This attacker will generate "<< m_goodIs << " Interests for existent contents per second");
  
  m_goodIs = (uint32_t) round(m_purity * m_frequency) ;

  Simulator::Schedule (Seconds(1.0), &CnmrWikiAttacker::ResetCounters, this);

}

void CnmrWikiAttacker::ScheduleNextPacket()
{

  if (m_firstTime)
  {
    m_sendEvent = Simulator::ScheduleNow (&CnmrWikiAttacker::SendPacket, this);
    m_firstTime = false;
  }
  else if(!m_sendEvent.IsRunning())
        m_sendEvent = Simulator::Schedule (Seconds(m_randomTime.GetValue()), &CnmrWikiAttacker::SendPacket, this);
}

void CnmrWikiAttacker::SendPacket ()
{
    NS_LOG_FUNCTION (this);

    uint32_t seq = GetNextSeq();

    std::string suffix;
    if (m_purity != 1.0 && m_goodIs > 0){
      suffix = CnmrWikiAttacker::readTrueElement(seq);
      m_goodIs--;
      goodIs++;
    }
    else{
      suffix = CnmrWikiAttacker::readFakeElement(seq);
      badIs++;
    }

    Ptr<Name> nameWithSequence = Create<Name> (m_prefixName);
    nameWithSequence->append (suffix);

    Ptr<Interest> interest = Create<Interest> ();
    interest->SetNonce               (m_randNonce.GetValue ());
    interest->SetName                (nameWithSequence);
    interest->SetInterestLifetime    (m_interestLifeTime);

    NS_LOG_INFO ("Requesting Interest: " << *interest);

    FwHopCountTag hopCountTag;
    hopCountTag.Add(GetNode()->GetId());
    interest->GetPayload ()->AddPacketTag (hopCountTag);

    m_transmittedInterests (interest, this, m_face);
    m_face->ReceiveInterest (interest);

    if(Simulator::Now() >= m_stopAt)
    {
        m_active = false;
    }
    else
    {
        ScheduleNextPacket();
    }
}

uint32_t CnmrWikiAttacker::GetNextSeq()
{
    return m_randomSeqId.GetValue();
}

} // namespace ndn
} // namespace ns3
