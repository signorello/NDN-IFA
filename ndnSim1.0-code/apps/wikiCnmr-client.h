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

#ifndef NDN_WIKICNMRCLIENT_H
#define NDN_WIKICNMRCLIENT_H

#include "wiki-client.h"
#include "ns3/ndnSIM/model/fw/monitor-aware-routing.h"
#include <vector>

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * \brief NDN application for TBD
 */
class WikiCnmrClient: public WikiClient
{
public:
  static TypeId GetTypeId ();

  /**
   * \brief Default constructor
   * Sets up randomizer function and packet sequence number
   */
  WikiCnmrClient ();
  virtual ~WikiCnmrClient () {};

  /**
   * \brief Method to specify a list of comma separated name-prefix the app may use
   * as prefixes for the issued Interests
   */
  void SetPrefixes(std::string p);

  void SetStartingIndex(uint32_t offset);

  // The three following are ndn.App baseClass methods

  void
  SendPacket ();

  uint32_t
  GetNextSeq();
  
protected:
  // from App
  virtual void
  StartApplication ();

  virtual void
  StopApplication ();

  void
  ScheduleNextPacket ();

private:
    // Pointer to the node fw strategy object
    Ptr<ndn::fw::MonitorAwareRouting> mar;
    
    UniformVariable     m_SeqRng; // Zpif-like consumer needs this, because they cannot reuse the random variable defined in the Consumer parent class
    uint32_t m_N;  //number of the contents
    uint32_t m_index_offset; // this values is used to position every client on a different part of the wikipedia bucket so as all can ask for different contents
    double m_q;  //q in (k+q)^s
    double m_s;  //s in (k+q)^s
    std::vector<double> m_Pcum;  //cumulative probability
    // Set&Get methods for the Zipf-M content selector
    void SetNumberOfContents (uint32_t numOfContents);
    uint32_t GetNumberOfContents () const;
    void SetQ (double q);
    double GetQ () const;
    void SetS (double s);
    double GetS () const;

    // Name-prefixes held by the node
    RandomVariable      *rngPrefix;
    std::vector<Name> prefixes;
    Name &GetNextPrefix();
/// @endcond
};

} // namespace ndn
} // namespace ns3

#endif
