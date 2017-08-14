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

#ifndef CNMR_WIKI_SMART_ATTACKER_H_
#define CNMR_WIKI_SMART_ATTACKER_H_

#include "cnmrWikiAttacker.h"
#include "ns3/ndn-name.h"
#include "ns3/nstime.h"

#include <string>
#include <vector>
#include <stdint.h>

namespace ns3 {
namespace ndn {

class CnmrWikiSmartAttacker : public CnmrWikiAttacker
{
public:
    static TypeId GetTypeId();

    CnmrWikiSmartAttacker();

protected:
    bool		m_switchT; // when on, the attacker switches target after m_observationPeriod passes
    Time m_observationPeriod;

    virtual void StartApplication ();
    virtual void StopApplication ();

    void ScheduleTargetChange();
    Name GetNextPrefix();
    void SetPrefixes(std::string);
   
private:
    std::vector<Name> m_prefixes;
    std::vector<Name>::iterator m_it;
};

} // namespace ndn
} // namespace ns3

#endif
