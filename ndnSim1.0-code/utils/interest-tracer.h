#ifndef INTEREST_TRACER_H
#define INTEREST_TRACER_H

#include "ns3/ptr.h"
#include "ns3/simple-ref-count.h"
#include <ns3/nstime.h>
#include <ns3/event-id.h>
#include <ns3/node-container.h>

#include "ns3/ndnSIM/model/fw/ndn-forwarding-strategy.h"

#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>
#include <map>
#include <list>

namespace ns3 {

class Node;
class Packet;

namespace ndn {

class Interest;
class Data;

typedef struct InterestReport {
      uint32_t received;
      uint32_t forwarded;
      uint32_t dropped;

    } InterestReport;

/**
 * @ingroup ndn-tracers
 * @brief NDN tracer for pit
 */
class InterestTracer : public SimpleRefCount<InterestTracer>
{
public:
  /**
   * @brief Helper method to install tracers on all simulation nodes
   *
   * @param file File to which traces will be written.  If filename is -, then std::out is used
   * @param averagingPeriod How often data will be written into the trace file (default, every half second)
   *
   * @returns a tuple of reference to output stream and list of tracers. !!! Attention !!! This tuple needs to be preserved
   *          for the lifetime of simulation, otherwise SEGFAULTs are inevitable
   * 
   */
  static void
  InstallAll (const std::string &file, Time averagingPeriod = Seconds (0.5));

  /**
   * @brief Helper method to install tracers on the selected simulation nodes
   *
   * @param nodes Nodes on which to install tracer
   * @param file File to which traces will be written.  If filename is -, then std::out is used
   * @param averagingPeriod How often data will be written into the trace file (default, every half second)
   *
   * @returns a tuple of reference to output stream and list of tracers. !!! Attention !!! This tuple needs to be preserved
   *          for the lifetime of simulation, otherwise SEGFAULTs are inevitable
   *
   */
  static void
  Install (const NodeContainer &nodes, const std::string &file, Time averagingPeriod = Seconds (0.5));

  /**
   * @brief Helper method to install tracers on a specific simulation node
   *
   * @param nodes Nodes on which to install tracer
   * @param file File to which traces will be written.  If filename is -, then std::out is used
   * @param averagingPeriod How often data will be written into the trace file (default, every half second)
   *
   * @returns a tuple of reference to output stream and list of tracers. !!! Attention !!! This tuple needs to be preserved
   *          for the lifetime of simulation, otherwise SEGFAULTs are inevitable
   *
   */
  static void
  Install (Ptr<Node> node, const std::string &file, Time averagingPeriod = Seconds (0.5));

  /**
   * @brief Helper method to install tracers on a specific simulation node
   *
   * @param nodes Nodes on which to install tracer
   * @param outputStream Smart pointer to a stream
   * @param averagingPeriod How often data will be written into the trace file (default, every half second)
   *
   * @returns a tuple of reference to output stream and list of tracers. !!! Attention !!! This tuple needs to be preserved
   *          for the lifetime of simulation, otherwise SEGFAULTs are inevitable
   */
  static Ptr<InterestTracer>
  Install (Ptr<Node> node, boost::shared_ptr<std::ostream> outputStream, Time averagingPeriod = Seconds (0.5));

  /**
   * @brief Explicit request to remove all statically created tracers
   *
   * This method can be helpful if simulation scenario contains several independent run,
   * or if it is desired to do a postprocessing of the resulting data
   */
  static void
  Destroy ();
  
  /**
   * @brief Trace constructor that attaches to the node using node pointer
   * @param os    reference to the output stream
   * @param node  pointer to the node
   */
  InterestTracer (boost::shared_ptr<std::ostream> os, Ptr<Node> node);

  /**
   * @brief Trace constructor that attaches to the node using node name
   * @param os        reference to the output stream
   * @param nodeName  name of the node registered using Names::Add
   */
  InterestTracer (boost::shared_ptr<std::ostream> os, const std::string &node);

  /**
   * @brief Destructor
   */
  ~InterestTracer ();

  /**
   * @brief Print head of the trace (e.g., for post-processing)
   *
   * @param os reference to output stream
   */
  void
  PrintHeader (std::ostream &os) const;

  /**
   * @brief Print current trace data
   *
   * @param os reference to output stream
   */
  void
  Print (std::ostream &os) const;
  
private:
  void
  Connect ();

  void
  InterestTypes (uint32_t type, uint32_t reportType);
 
  void
  SetAveragingPeriod (const Time &period);

  void
  Reset ();

  void
  PeriodicPrinter ();
  
private:
  std::string m_node;
  Ptr<Node> m_nodePtr;

  Ptr<ns3::ndn::ForwardingStrategy> stg_ptr;
  std::string stg_type;

  // This array only contains two elements, report about malicious interests in the '0' cell and legitimate ones in the '1' cell
  InterestReport interest_reports[2];

  boost::shared_ptr<std::ostream> m_os;

  Time m_period;
  EventId m_printEvent;
};

/**
 * @brief Helper to dump the trace to an output stream
 */
inline std::ostream&
operator << (std::ostream &os, const InterestTracer &tracer)
{
  os << "# ";
  tracer.PrintHeader (os);
  os << "\n";
  tracer.Print (os);
  return os;
}

} // namespace ndn
} // namespace ns3

#endif // PIT_TRACER_H
