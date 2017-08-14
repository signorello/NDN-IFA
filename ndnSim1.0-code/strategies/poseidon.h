/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011-2013 University of California, Los Angeles
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
 * Author: Salvatore Signorello <salvatore.signorello@uni.lu>
 */

#include "ndn-forwarding-strategy.h"
#include "ns3/nstime.h"
#include "ns3/log.h"
#include "ns3/ndn-name.h"
#include "ns3/traced-callback.h"
#include <map> 
#include <utility>

namespace ns3 {
namespace ndn {
namespace fw {

typedef struct Isr {
      uint32_t interestsReceived;
      uint32_t contentsReceived;
      double ratio;

      Isr():interestsReceived(0),contentsReceived(0),ratio(0.00){}

      void Reset(){
	interestsReceived = contentsReceived = 0;
      	ratio = 0.00;
      }

    } Isr;

// The poseidon's paper talks about the pit size in MB taken by traffic coming from
// each interface. Our workaround simply counts the entries and divides those 
// by the max pit Size
typedef struct Pur {
      uint32_t pitEntries;
      double ratio;

      Pur():pitEntries(0),ratio(0){}

      void Reset(){
	pitEntries = 0;
	ratio = 0;
      }

    } Pur;

typedef std::pair<Isr, Pur>  IsrPurPair;
typedef std::map<Ptr<Face>, IsrPurPair> PerFaceStats;
typedef std::map<Ptr<Face>, std::pair<double,double>> PerFaceThresholds;
// Let's associate two timers at each interface. The first one relates to
// the alarms sent through that interface, while the second relates to the
// ones received.
typedef std::map<Ptr<Face>, std::pair<Time,Time>> PerFaceTimes;


class Poseidon : public ForwardingStrategy
{
private:
    typedef ForwardingStrategy super;

public:
  static TypeId GetTypeId ();

  /**
   * @brief Default constructor
   */
  Poseidon ();

  static std::string GetLogName ();


  double getPitUsage();
  
  virtual void
  OnInterest (Ptr<Face> face,
              Ptr<Interest> interest);

  virtual void
  OnData (Ptr<Face> face,
          Ptr<Data> data);

  virtual void
  AddFace (Ptr<Face> face);
  
private:
  void
  GeneratePBalarm (Ptr<Face> face, double scale);

  void
  ResetStats ();

  void
  InitThresholds();

  void
  ScaleThresholds (Ptr<Face> inFace,
                       Ptr<const Interest> interest);
  void
  CreateAlertsName(std::string);

protected:
  // from Object
  virtual void
  NotifyNewAggregate (); ///< @brief Even when object is aggregated to another Object

  // This is pure virtual in the base fw strategy
  virtual bool DoPropagateInterest (Ptr<Face> inFace, Ptr<const Interest> interest, Ptr<pit::Entry> pitEntry);

  virtual void
  DidSendOutInterest (Ptr<Face> inFace,
                      Ptr<Face> outFace,
                      Ptr<const Interest> interest,
                      Ptr<pit::Entry> pitEntry);

  virtual void
  SatisfyPendingInterest (Ptr<Face> inFace, // 0 allowed (from cache)
                          Ptr<const Data> data,
                          Ptr<pit::Entry> pitEntry);

  virtual void
  DidCreatePitEntry (Ptr<Face> inFace,
                     Ptr<const Interest> interest,
                     Ptr<pit::Entry> pitEntry);

  virtual void
  DoDispose ();
    

  TracedCallback<double, uint32_t> pitUsageTrace; // this is needed for the PIT tracer
private:
  static LogComponent g_log;
  
  bool m_resetStatsScheduled;

  bool m_distributed; // when on, the class implements the distributed version of Poseidon, if not it just operates locally. It is on by default

  Ptr<Name> m_alerts_prefix; // reserved name space used to transmit the pushback alerts

  PerFaceStats m_stats; // this is meant to track ISR and PUR values which are compared with the tresholds omega and rho at each Interest reception
  PerFaceThresholds m_thres; 
  PerFaceTimes m_times;

  double m_omega; // threshold for the ISR-like metric
  double m_rho; // threshold for the PIT usage
  double m_scale; // scale factor to be sent within an alert out of interfaces

  Time m_timeInterval; // time window during which statistics must be counted
  Time m_waitTime; // minimum notifications inter-gap

  int pitMaxSize;
  uint32_t m_virtualPayloadSize; // payload of the pushback data alert

  TracedCallback< Ptr<const Face>, double /* ratio */, double/* adjusted weight */, double /* limit */ > m_onLimitsAnnounce;
};


} // namespace fw
} // namespace ndn
} // namespace ns3
