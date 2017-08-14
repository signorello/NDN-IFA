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

#ifndef CNMR_WIKI_ATTACKER_H_
#define CNMR_WIKI_ATTACKER_H_

#include "ndn-app.h"
#include "ns3/random-variable.h"
#include "ns3/ndn-name.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/ndn-rtt-estimator.h"

#include <string>
#include <vector>
#include <stdint.h>
#include "wiki-client.h"

namespace ns3 {
namespace ndn {

class CnmrWikiAttacker : public App
{
public:
    static const uint32_t FLAG = 600;

    static TypeId GetTypeId();

    CnmrWikiAttacker();

protected:
    double              m_frequency; // Frequency of interest packets
    double		m_purity; // Percentage of existing contents requested per sec - 1.0 means all fake Interests
    uint32_t		m_goodIs; // number of existent contents to be requested per second
    // let me add two convenience counters to check the right num of bad/good interests are emitted each second
    uint32_t goodIs;
    uint32_t badIs;

    // Random number generator for content IDs
    UniformVariable     m_randomSeqId;

    // Random number generator for inter-interest gaps
    RandomVariable      m_randomTime;

    // nonce generator
    UniformVariable m_randNonce;

    bool m_firstTime;
    Time m_startAt;
    Time m_stopAt;
    Time m_interestLifeTime;

    std::string m_fakeList;
    uint32_t m_sizeFakeList;
    std::vector<std::string> m_fakeSuffixes;

    EventId m_sendEvent;
    Name m_prefixName;

    virtual void StartApplication ();
    virtual void StopApplication ();

    /**
     * \brief Constructs the Interest packet and sends it using a callback to the underlying NDN protocol
     */
    virtual void ScheduleNextPacket ();

    /**
     * @brief Actually send packet
     */
    void SendPacket ();

    void loadFakePrefixesList();
    void ResetCounters();
    std::string readFakeElement(uint32_t index);
    std::string readTrueElement(uint32_t index);
    uint32_t GetNextSeq();
};

} // namespace ndn
} // namespace ns3

#endif
