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
 */

#include "ndn-consumer.h"
#include "cnmrWikiSmartAttacker.h"
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
#include <boost/algorithm/string.hpp>

#include <fstream>
#include <iostream>
#include <math.h>

NS_LOG_COMPONENT_DEFINE ("ndn.CnmrWikiSmartAttacker");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED (CnmrWikiSmartAttacker);

TypeId CnmrWikiSmartAttacker::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::ndn::CnmrWikiSmartAttacker")
        .SetGroupName ("Ndn")
        .SetParent<CnmrWikiAttacker> ()
        .AddConstructor<CnmrWikiSmartAttacker> ()

        .AddAttribute ("Prefixes","Comma separated list of target prefix names",
                StringValue ("wikipedia.org"),
                MakeStringAccessor (&CnmrWikiSmartAttacker::SetPrefixes),
                MakeStringChecker ())

	.AddAttribute ("switchTarget",
		"This value represents the percentage of existent contents requested each second",
                BooleanValue(true),
                MakeBooleanAccessor (&CnmrWikiSmartAttacker::m_switchT),
                MakeBooleanChecker ())
	.AddAttribute("ObservationPeriod", "Interval at which a monitor reports to the CC",
                TimeValue (Seconds (10)),
                MakeTimeAccessor (&CnmrWikiSmartAttacker::m_observationPeriod),
                MakeTimeChecker ());
        ;
    return tid;
}

CnmrWikiSmartAttacker::CnmrWikiSmartAttacker()
{}

void CnmrWikiSmartAttacker::StartApplication()
{
    NS_LOG_FUNCTION_NOARGS ();

    CnmrWikiAttacker::StartApplication();
    m_prefixName = GetNextPrefix();

    // schedule the prefix change after the attack if the switchT flag is on
    // The prefix is scheduled after the attack starts
    if (m_switchT){
      Simulator::Schedule (m_startAt + m_observationPeriod, &CnmrWikiSmartAttacker::ScheduleTargetChange, this);
    }

}

void CnmrWikiSmartAttacker::SetPrefixes(std::string strPrefixes)
{
    std::vector<std::string> strs;
    boost::split(strs, strPrefixes, boost::is_any_of(","));
    m_prefixes.clear();

    for(size_t i = 0; i < strs.size(); i++)
    {
        m_prefixes.push_back(Name(strs[i]));
    }

    NS_LOG_INFO ("Targets are " << strPrefixes);
    m_it = m_prefixes.begin();
}

Name CnmrWikiSmartAttacker::GetNextPrefix()
{
    if(++m_it == m_prefixes.end())
      m_it = m_prefixes.begin();

    return *m_it;
    // I use the above code since the next single line generate a std:alloc which
    // I do not have time now to investigate this - TODO
    //return (++m_it != m_prefixes.end()) ? *m_it : *m_prefixes.begin();
}

void CnmrWikiSmartAttacker::StopApplication ()
{
    NS_LOG_FUNCTION_NOARGS ();

    CnmrWikiSmartAttacker::StopApplication ();
}

void CnmrWikiSmartAttacker::ScheduleTargetChange()
{
    NS_LOG_INFO ("Old target was " << m_prefixName );

    m_prefixName = GetNextPrefix();

    NS_LOG_INFO ("Changing target to " << m_prefixName );
    
    Simulator::Schedule (m_observationPeriod, &CnmrWikiSmartAttacker::ScheduleTargetChange, this);
}

} // namespace ndn
} // namespace ns3
