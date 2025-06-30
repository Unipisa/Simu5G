//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef MULTIHOPD2DSTATISTICS_H_
#define MULTIHOPD2DSTATISTICS_H_

#include "common/LteCommon.h"
#include "common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

//
// MultihopD2DStatistics module
//
// This module is only used to gather statistics related to events generated
// by the Multihop D2D apps. It is accessed via method calls.
//
// Every time a MultihopD2D app generate a new message (notified by the
// EventGenerator), it register the new event by calling the recordNewBroadcast() function.
// An event is associated with a *target area*, i.e. the set of nodes that should
// receive the notification of the event.
// Every node that receives the message, notifies the event to this module, in
// order to update statistics. For example, when a node receives a message, it
// calls the recordReception() function.
//
class MultihopD2DStatistics : public cSimpleModule
{

    // the delivery status of an event is composed of the reception status
    // at nodes comprised within the target area of the event. For each node,
    // the reception status includes the reception delay and number of hops.
    // If delay < 0, the node has not (yet) received the message
    struct ReceptionStatus {
        simtime_t delay_;
        int hops_;
    };
    typedef std::map<MacNodeId, ReceptionStatus> DeliveryStatus;

    // for each event, store the current delivery status
    std::map<unsigned short, DeliveryStatus> eventDeliveryInfo_;

    // for each event, store the number of transmissions/duplicates/suppressions
    struct TransmissionInfo
    {
        unsigned int numSent_ = 0;
        unsigned int numSuppressed_ = 0;
        unsigned int numDuplicates_ = 0;

        TransmissionInfo()  { }
    };
    std::map<unsigned short, TransmissionInfo> eventTransmissionInfo_;

    // statistics
    static simsignal_t d2dMultihopEventDelaySignal_;           // average reception delay of one event within the target area
    static simsignal_t d2dMultihopEventDelay95PerSignal_;      // latency required to cover the 95% of nodes within the target area
    static simsignal_t d2dMultihopEventDeliveryRatioSignal_;   // percentage of nodes covered within the target area
    static simsignal_t d2dMultihopEventSentMsgSignal_;         // number of transmitted messages for broadcasting an event within the target area
    static simsignal_t d2dMultihopEventTrickleSuppressedMsgSignal_;   // number of message relaying suppressed by the Trickle algorithm (if enabled)
    static simsignal_t d2dMultihopEventRcvdDupMsgSignal_;      // number of duplicates within the target area
    static simsignal_t d2dMultihopEventCompleteDeliveriesSignal_;    // percentage of clusters completely-covered within the target area

  protected:

    void initialize() override;
    void finish() override;

  public:
    void recordNewBroadcast(unsigned int msgId, UeSet& destinations);
    void recordReception(MacNodeId nodeId, unsigned int msgId, simtime_t delay, int hops);
    void recordSentMessage(unsigned int msgId);
    void recordSuppressedMessage(unsigned int msgId);
    void recordDuplicateReception(unsigned int msgId);
};

} //namespace

#endif /* MULTIHOPD2DSTATISTICS_H_ */

