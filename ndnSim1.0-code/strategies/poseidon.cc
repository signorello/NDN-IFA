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

#include "poseidon.h"

#include "ns3/ndn-l3-protocol.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndn-pit.h"
#include "ns3/ndn-pit-entry.h"

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/random-variable.h"
#include "ns3/double.h"
#include "ns3/string.h"

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
namespace ll = boost::lambda;

namespace ns3 {
namespace ndn {
namespace fw {

NS_OBJECT_ENSURE_REGISTERED (Poseidon);
// TODOs
// - check where it is more appropriate to initialize the ResetStats method
// in other classes this kind of event happens to be done inside the NotifyNewAggregate
// while for me it makes more sense inside the class constructor

LogComponent Poseidon::g_log = LogComponent (Poseidon::GetLogName ().c_str ());

TypeId
Poseidon::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ndn::fw::Poseidon")
    .SetGroupName ("Ndn")
    .SetParent <ForwardingStrategy> ()
    .AddConstructor <Poseidon> ()
    .AddTraceSource ("PITUsage",  "PITUsage",  MakeTraceSourceAccessor (&Poseidon::pitUsageTrace))
    .AddAttribute("distributed", "Enable/Disable collaboration among routers",
       BooleanValue(false),
       MakeBooleanAccessor (&Poseidon::m_distributed),
       MakeBooleanChecker ())
    .AddAttribute("ScaleFactor", "Factor used to lower omega and rho thresholds",
       StringValue("0.5"),
       MakeDoubleAccessor (&Poseidon::m_scale),
       MakeDoubleChecker<double> ())
    .AddAttribute("OmegaThreshold", "Initial threshold value for the ISR",
       StringValue("3.0"),
       MakeDoubleAccessor (&Poseidon::m_omega),
       MakeDoubleChecker<double> ())
    .AddAttribute("RhoThreshold", "Initial threshold value for the PIT usage expressed as percentage of the max pit size",
       StringValue("0.125"),
       MakeDoubleAccessor (&Poseidon::m_rho),
       MakeDoubleChecker<double> ())
    .AddAttribute ("AlertsPrefix", "Prefix used to pushback alert notifications",
       StringValue ("/pushback/alerts"),
       MakeStringAccessor (&Poseidon::CreateAlertsName),
       MakeStringChecker ())
    .AddAttribute ("Time Window", "Monitoring interval duration",
      StringValue ("10s"),
      MakeTimeAccessor (&Poseidon::m_timeInterval),
      MakeTimeChecker ())
    .AddAttribute ("Wait time", "Minimum inter-gap between two alerts at the same interface",
      StringValue ("60ms"),
      MakeTimeAccessor (&Poseidon::m_waitTime),
      MakeTimeChecker ())
    ;
  return tid;
}

// Inserting a constructor since I do not see any better place to read the PIT maxSize value
Poseidon::Poseidon ()
{  
  m_resetStatsScheduled = false;
  m_virtualPayloadSize = 0;
}

void
Poseidon::DoDispose ()
{  
  super::DoDispose ();
}

// it is still no crystal clear why this kind of initialization has to go inside the 
// NotifyNewAggregate method, so I am just following what implemented by other strategies
void
Poseidon::NotifyNewAggregate ()
{
  super::NotifyNewAggregate ();

  if (!m_resetStatsScheduled)
    {
      if (this->m_pit != 0 && this->m_fib != 0 && this->template GetObject<Node> () != 0)
        {
          m_resetStatsScheduled = true;

	  StringValue svPitSize;
  	  this->m_pit->GetAttribute("MaxSize", svPitSize);
  	  pitMaxSize = atoi(svPitSize.Get().c_str());

          Simulator::ScheduleWithContext (this->template GetObject<Node> ()->GetId (),
                                          m_timeInterval, &Poseidon::ResetStats, this);
        }
    }
}

std::string
Poseidon::GetLogName ()
{
  return super::GetLogName ()+".Poseidon";
}

void
Poseidon::CreateAlertsName(std::string prefix)
{
  m_alerts_prefix = Create<Name> (prefix);
}

void
Poseidon::AddFace (Ptr<Face> face)
{
  super::AddFace (face);

  NS_LOG_INFO("Setting thresholds on " << face );
  // Add entries for this face to the stats, times and thresholds maps
  m_stats[face] = std::make_pair(*(new Isr()),*(new Pur()));

  m_thres[face] = std::make_pair(m_omega,m_rho*(double)pitMaxSize);

  Time now =  Simulator::Now ();
  m_times[face] = std::make_pair(now,now);

}

void
Poseidon::OnInterest (Ptr<Face> face,
                                               Ptr<Interest> interest)
{
  // The first thing is to load the stats for this face
  IsrPurPair *face_stats = &(m_stats.find(face)->second) ;
  Isr *isr = &(face_stats->first);
  Pur *pur = &(face_stats->second);

  // then load the threshold values for this face too
  // for the following you don't need to use refs since you only perform read ops
  std::pair<double,double> thresholds = m_thres.find(face)->second;
  double local_omega = thresholds.first;
  double pit_fraction = thresholds.second;
  
  NS_LOG_DEBUG ("ISR " << isr->ratio << " - omega " << local_omega);
  NS_LOG_DEBUG ("PitEntries " << pur->pitEntries << " - rho*PITsize " << pit_fraction);
  if(isr->ratio > local_omega && pur->pitEntries > pit_fraction){
    std::pair<Time,Time> *times = &(m_times.find(face)->second);
    Time *last_alert = &(times->first); 
    Time now =  Simulator::Now ();
    if((now - *last_alert) > m_waitTime){
       NS_LOG_INFO("Time elapsed since last PushBack was emitted " << (now - *last_alert) );
       // if this does not come from an AppFace, issue a pushback; otherwise simply drop
       if(face->GetInstanceTypeId ().GetName () != "ns3::ndn::AppFace"){
          GeneratePBalarm (face, m_scale);
          last_alert = &now;
       }
       else
	  NS_LOG_DEBUG ("Next step is the app, no pushback but drop ");
    }
    else{
      // in this case the interest has to be dropped
      // ideally I would add a specific tracer to keep track of these Interests
      // assuming the ones written for the parent class cannot be easily leveraged

      NS_LOG_INFO("Dropping Interest, but no PushBack notification on  " << face );
      return;
    }
  }

  // just call the parent class routine to forward this and overload the method DidSendOutInterest to increment our stats counters
  super::OnInterest(face,interest);
  
  pitUsageTrace(getPitUsage(), this->m_pit->GetSize());
}

double
Poseidon::getPitUsage()
{
  // If the PIT size is not restricted return -1
  return pitMaxSize == 0 ? -1 : (double)this->m_pit->GetSize() / (double)pitMaxSize;
}

bool
Poseidon::DoPropagateInterest (Ptr<Face> inFace, Ptr<const Interest> interest, Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (this << interest->GetName ());
  
  int propagatedCount = 0;
  
  BOOST_FOREACH (const fib::FaceMetric &metricFace, pitEntry->GetFibEntry ()->m_faces.get<fib::i_metric> ())
    {
      NS_LOG_DEBUG ("Trying " << boost::cref(metricFace));
      if (metricFace.GetStatus () == fib::FaceMetric::NDN_FIB_RED) // all non-read faces are in front
        break;
  
      if (!TrySendOutInterest (inFace, metricFace.GetFace (), interest, pitEntry))
        {
          continue;
        }
  
      propagatedCount++;
      break; // do only once
    }
  
  NS_LOG_INFO ("Propagated to " << propagatedCount << " faces");
  return propagatedCount > 0;
}

void
Poseidon::DidSendOutInterest (Ptr<Face> inFace,
                                   Ptr<Face> outFace,
                                   Ptr<const Interest> interest,
                                   Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (inFace);
  super::DidSendOutInterest (inFace, outFace, interest, pitEntry);

  IsrPurPair *face_stats = &(m_stats.find(inFace)->second) ;
  Isr *isr = &(face_stats->first);
  NS_LOG_DEBUG ("Interests " << isr->interestsReceived << " Contents " << isr->contentsReceived);
  isr->interestsReceived++;
  double new_ratio = (isr->contentsReceived != 0) ? (isr->interestsReceived/(double) isr->contentsReceived) : 0;
  isr->ratio = new_ratio;

  NS_LOG_DEBUG ("New ISR ratio " << new_ratio);

}

void
Poseidon::DidCreatePitEntry (Ptr<Face> inFace,
                     Ptr<const Interest> interest,
                     Ptr<pit::Entry> pitEntry)
{
  NS_LOG_FUNCTION (inFace);

  NS_LOG_DEBUG ("Updating PUR stats on " << *inFace);
  IsrPurPair *face_stats = &(m_stats.find(inFace)->second) ;
  Pur *pur = &(face_stats->second);
  pur->pitEntries++;
  pur->ratio = pur->pitEntries/(double)pitMaxSize;

  NS_LOG_DEBUG ("PitEntries " << pur->pitEntries << " PUR "<< pur->ratio);
}

void
Poseidon::GeneratePBalarm (Ptr<Face> face, double scale){

  // we cheat a bit at this stage, since we do not send any scale factor back, rather we
  // assume every router use the same value which is specified so far as class attribute
  
  NS_LOG_INFO("Emitting PushBack notification on  " << face );

  Ptr<Data> data = Create<Data> (Create<Packet> (m_virtualPayloadSize));
  data->SetName (m_alerts_prefix);
  data->SetTimestamp (Simulator::Now ());
  data->SetFreshness (Seconds(0));
  data->SetSignature (12345);
  face->SendData (data);

  // Do I need to set any route for those alarms to be forwarded correctly?
}

void
Poseidon::ResetStats ()
{
  NS_LOG_FUNCTION ("Resetting stats for all faces");
  PerFaceStats::iterator it;
  for(it = m_stats.begin(); it != m_stats.end(); ++it){
    IsrPurPair *face_stats = &(it->second);
    face_stats->first.Reset();
    face_stats->second.Reset();
  }

  Simulator::Schedule (m_timeInterval, &Poseidon::ResetStats, this);
}

void
Poseidon::OnData (Ptr<Face> face, Ptr<Data> data)
{

  NS_LOG_FUNCTION (face << data->GetName ());
  // it is important to understand when to update statistics, I'd say whenever
  // a PIT entry is satisfied, so in the SatisfyPendingInterest (inFace, data, pitEntry);

  Name data_name = data->GetName ();

  if(data_name.compare(*m_alerts_prefix) == 0){

    NS_LOG_INFO("Received Pushback notification on  " << face );
    std::pair<Time,Time> *times = &(m_times.find(face)->second);
    Time *last_alert_received = &(times->second);
    Time now = Simulator::Now ();

    if ((now - *last_alert_received) > m_waitTime){

       NS_LOG_INFO("Pushback processed for " << face );
       // sufficient time has elapsed since the last alert
       // so this can be accepted and thresholds should be lowered down 
       std::pair<double,double> *thresholds = &(m_thres.find(face)->second);
       thresholds->first*=m_scale;
       thresholds->second*=m_scale;

       last_alert_received = &now;

    }
    else{
      // otherwise, just ignore the alert
      NS_LOG_INFO("Pushback refused on " << face << " cause too close to the previous one " );
      return;
    }

  }
  else {
      NS_LOG_INFO("This is normal data to be processed regularly");
      super::OnData(face,data);
  }
  
}

// This function is really critical, since the Face where you get the Data is not the same
// as the one you got the interest. This means that the lookup on the m_stats must be done 
// by using a reference to the face.
void
Poseidon::SatisfyPendingInterest (Ptr<Face> inFace,Ptr<const Data> data,Ptr<pit::Entry> pitEntry)
{

  NS_LOG_FUNCTION (inFace << data->GetName ());

  BOOST_FOREACH (const pit::IncomingFace &incoming, pitEntry->GetIncoming ())
  {
      NS_LOG_DEBUG ("Updating stats on " << *incoming.m_face);
  
      IsrPurPair *face_stats = &(m_stats.find(incoming.m_face)->second);
      Isr *isr = &(face_stats->first);
      NS_LOG_DEBUG ("Interests " << isr->interestsReceived << " Contents " << isr->contentsReceived);
      isr->contentsReceived++;
      double new_ratio = (isr->contentsReceived != 0) ? (isr->interestsReceived/(double) isr->contentsReceived) : 0;
      isr->ratio = new_ratio;
      
      Pur *pur = &(face_stats->second);
      NS_LOG_DEBUG ("PitEntries " << pur->pitEntries << " Ratio " << pur->ratio);
      
      //NS_ASSERT_MSG((pur->pitEntries != 0 ), "PitEntries cannot be decremented when is zero");
      if(pur->pitEntries != 0)
	pur->pitEntries--;

      pur->ratio = pur->pitEntries/(double) pitMaxSize;
  
  }

  // I call this as last operation, since it includes clearing PIT entries content
  // and I don't want to risk to access sth here which has already been cleared there
  super::SatisfyPendingInterest(inFace, data, pitEntry);

  // SMALL MEMO for the future.
  // Indeed, the pit size comes from the Policy the PIT implements. A policy is the
  // template classe the PIT is instantiated with.
  // super::getPolicy ().size ();

}


} // namespace fw
} // namespace ndn
} // namespace ns3
