#include "interest-tracer.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/config.h"
#include "ns3/names.h"
#include "ns3/callback.h"

#include "ns3/ndn-app.h"
#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/simulator.h"
#include "ns3/node-list.h"
#include "ns3/log.h"

#include <boost/lexical_cast.hpp>

#include <fstream>

NS_LOG_COMPONENT_DEFINE ("ndn.InterestTracer");

using namespace std;

namespace ns3 {
namespace ndn {

static std::list< boost::tuple< boost::shared_ptr<std::ostream>, std::list<Ptr<InterestTracer> > > > g_tracers;

template<class T>
static inline void
NullDeleter (T *ptr)
{
}

void
InterestTracer::Destroy ()
{
    g_tracers.clear ();
}

    void
InterestTracer::InstallAll (const std::string &file, Time averagingPeriod/* = Seconds (0.5)*/)
{
    using namespace boost;
    using namespace std;

    std::list<Ptr<InterestTracer> > tracers;
    boost::shared_ptr<std::ostream> outputStream;
    if (file != "-")
    {
        boost::shared_ptr<std::ofstream> os (new std::ofstream ());
        os->open (file.c_str (), std::ios_base::out | std::ios_base::trunc);

        if (!os->is_open ())
        {
            NS_LOG_ERROR ("File " << file << " cannot be opened for writing. Tracing disabled");
            return;
        }

        outputStream = os;
    }
    else
    {
        outputStream = boost::shared_ptr<std::ostream> (&std::cout, NullDeleter<std::ostream>);
    }

    for (NodeList::Iterator node = NodeList::Begin ();
            node != NodeList::End ();
            node++)
    {
        Ptr<InterestTracer> trace = Install (*node, outputStream, averagingPeriod);
        tracers.push_back (trace);
    }

    if (tracers.size () > 0)
    {
        tracers.front ()->PrintHeader (*outputStream);
        *outputStream << "\n";
    }

    g_tracers.push_back (boost::make_tuple (outputStream, tracers));
}

    void
InterestTracer::Install (const NodeContainer &nodes, const std::string &file, Time averagingPeriod/* = Seconds (0.5)*/)
{
    using namespace boost;
    using namespace std;

    std::list<Ptr<InterestTracer> > tracers;
    boost::shared_ptr<std::ostream> outputStream;
    if (file != "-")
    {
        boost::shared_ptr<std::ofstream> os (new std::ofstream ());
        os->open (file.c_str (), std::ios_base::out | std::ios_base::trunc);

        if (!os->is_open ())
        {
            NS_LOG_ERROR ("File " << file << " cannot be opened for writing. Tracing disabled");
            return;
        }

        outputStream = os;
    }
    else
    {
        outputStream = boost::shared_ptr<std::ostream> (&std::cout, NullDeleter<std::ostream>);
    }

    for (NodeContainer::Iterator node = nodes.Begin ();
            node != nodes.End ();
            node++)
    {
        Ptr<InterestTracer> trace = Install (*node, outputStream, averagingPeriod);
        tracers.push_back (trace);
    }

    if (tracers.size () > 0)
    {
        // *m_l3RateTrace << "# "; // not necessary for R's read.table
        tracers.front ()->PrintHeader (*outputStream);
        *outputStream << "\n";
    }

    g_tracers.push_back (boost::make_tuple (outputStream, tracers));
}

    void
InterestTracer::Install (Ptr<Node> node, const std::string &file, Time averagingPeriod/* = Seconds (0.5)*/)
{
    using namespace boost;
    using namespace std;

    std::list<Ptr<InterestTracer> > tracers;
    boost::shared_ptr<std::ostream> outputStream;
    if (file != "-")
    {
        boost::shared_ptr<std::ofstream> os (new std::ofstream ());
        os->open (file.c_str (), std::ios_base::out | std::ios_base::trunc);

        if (!os->is_open ())
        {
            NS_LOG_ERROR ("File " << file << " cannot be opened for writing. Tracing disabled");
            return;
        }

        outputStream = os;
    }
    else
    {
        outputStream = boost::shared_ptr<std::ostream> (&std::cout, NullDeleter<std::ostream>);
    }

    Ptr<InterestTracer> trace = Install (node, outputStream, averagingPeriod);
    tracers.push_back (trace);

    if (tracers.size () > 0)
    {
        // *m_l3RateTrace << "# "; // not necessary for R's read.table
        tracers.front ()->PrintHeader (*outputStream);
        *outputStream << "\n";
    }

    g_tracers.push_back (boost::make_tuple (outputStream, tracers));
}


    Ptr<InterestTracer>
InterestTracer::Install (Ptr<Node> node,
        boost::shared_ptr<std::ostream> outputStream,
        Time averagingPeriod/* = Seconds (0.5)*/)
{
    NS_LOG_DEBUG ("Node: " << node->GetId ());

    Ptr<InterestTracer> trace = Create<InterestTracer> (outputStream, node);
    trace->SetAveragingPeriod (averagingPeriod);

    return trace;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

    InterestTracer::InterestTracer (boost::shared_ptr<std::ostream> os, Ptr<Node> node)
    : m_nodePtr (node)
      , m_os (os)
{
    m_node = boost::lexical_cast<string> (m_nodePtr->GetId ());

    Connect ();

    string name = Names::FindName (node);
    if (!name.empty ())
    {
        m_node = name;
    }
}

    InterestTracer::InterestTracer (boost::shared_ptr<std::ostream> os, const std::string &node)
    : m_node (node)
      , m_os (os)
{
    Connect ();
}

InterestTracer::~InterestTracer ()
{
};


    void
InterestTracer::Connect ()
{
  bool gotStr = false;
  stg_ptr = m_nodePtr->GetObject<ns3::ndn::ForwardingStrategy> ();

  if(stg_ptr != 0 ){
      stg_ptr->TraceConnectWithoutContext ("InterestTypes", MakeCallback (&InterestTracer::InterestTypes, this));
  
      gotStr = true;
  }

  if(!gotStr){
      NS_LOG_ERROR ("No strategy detected: " << stg_type);
  }
    
  Reset();
}


    void
InterestTracer::SetAveragingPeriod (const Time &period)
{
    m_period = period;
    m_printEvent.Cancel ();
    m_printEvent = Simulator::Schedule (m_period, &InterestTracer::PeriodicPrinter, this);
}

    void
InterestTracer::PeriodicPrinter ()
{
    Print (*m_os);
    Reset ();

    m_printEvent = Simulator::Schedule (m_period, &InterestTracer::PeriodicPrinter, this);
}

void
InterestTracer::PrintHeader (std::ostream &os) const
{
    os << "Time" << "\t"
        << "Node" << "\t"
        << "Type" << "\t"
        << "Signal" << "\t"
        << "Value" << "\t";
}

void
InterestTracer::Reset ()
{
  for (InterestReport &interest_report : interest_reports){
    interest_report.received = 0;
    interest_report.forwarded = 0;
    interest_report.dropped = 0;
  }
}

#define PRINTER(type, printName, fieldName)           \
  os << time.ToDouble (Time::S) << "\t"         \
  << m_node << "\t"                             \
  << type << "\t"				\
  << printName << "\t"                          \
  << fieldName << "\n";

void
InterestTracer::Print (std::ostream &os) const
{
    Time time = Simulator::Now ();

    // Fake Interests
    PRINTER (0, "Received", interest_reports[0].received);
    PRINTER (0, "Forwarded", interest_reports[0].forwarded);
    PRINTER (0, "Dropped", interest_reports[0].dropped);

    // Valid Interests
    PRINTER (1, "Received", interest_reports[1].received);
    PRINTER (1, "Forwarded", interest_reports[1].forwarded);
    PRINTER (1, "Dropped", interest_reports[1].dropped);
}

void InterestTracer::InterestTypes (uint32_t type, uint32_t reportType)
{
    this->interest_reports[type].received++;

    if( reportType == 1 )
      this->interest_reports[type].forwarded++;
    else if ( reportType == 0 )
      this->interest_reports[type].dropped++;
}

} // namespace ndn
} // namespace ns3
