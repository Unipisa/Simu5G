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

#include "apps/d2dMultihop/statistics/MultihopD2DStatistics.h"
#include "apps/d2dMultihop/MultihopD2D.h"

namespace simu5g {

Define_Module(MultihopD2DStatistics);

using namespace omnetpp;

// register statistics
simsignal_t MultihopD2DStatistics::d2dMultihopEventDeliveryRatioSignal_ = registerSignal("d2dMultihopEventDeliveryRatio");
simsignal_t MultihopD2DStatistics::d2dMultihopEventDelaySignal_ = registerSignal("d2dMultihopEventDelay");
simsignal_t MultihopD2DStatistics::d2dMultihopEventDelay95PerSignal_ = registerSignal("d2dMultihopEventDelay95Per");
simsignal_t MultihopD2DStatistics::d2dMultihopEventSentMsgSignal_ = registerSignal("d2dMultihopEventSentMsg");
simsignal_t MultihopD2DStatistics::d2dMultihopEventTrickleSuppressedMsgSignal_ = registerSignal("d2dMultihopEventTrickleSuppressedMsg");
simsignal_t MultihopD2DStatistics::d2dMultihopEventRcvdDupMsgSignal_ = registerSignal("d2dMultihopEventRcvdDupMsg");
simsignal_t MultihopD2DStatistics::d2dMultihopEventCompleteDeliveriesSignal_ = registerSignal("d2dMultihopEventCompleteDeliveries");

void MultihopD2DStatistics::initialize()
{
}

void MultihopD2DStatistics::recordNewBroadcast(unsigned int msgId, UeSet& destinations)
{
    // consider the least-significant 16 bits
    unsigned short eventId = (unsigned short)msgId;
    if (eventDeliveryInfo_.find(eventId) == eventDeliveryInfo_.end()) {
        // initialize record for this message
        DeliveryStatus tmp;
        for (const auto& destination : destinations) {
            ReceptionStatus status;
            status.delay_ = -1.0;
            status.hops_ = -1;
            tmp.insert({destination, status});
        }
        std::pair<unsigned short, DeliveryStatus> p(eventId, tmp);
        eventDeliveryInfo_.insert(p);
    }
    else {
        DeliveryStatus entry = eventDeliveryInfo_.at(eventId);
        for (const auto& destination : destinations) {
            if (eventDeliveryInfo_[eventId].find(destination) == eventDeliveryInfo_[eventId].end()) {
                // the UE is not in the map
                ReceptionStatus status;
                status.delay_ = -1.0;
                status.hops_ = -1;
                eventDeliveryInfo_[eventId].insert({destination, status});
            }
        }
    }

    if (eventTransmissionInfo_.find(eventId) == eventTransmissionInfo_.end()) {
        TransmissionInfo info;
        info.numSent_ = 1;
        eventTransmissionInfo_[eventId] = info;
    }
}

void MultihopD2DStatistics::recordReception(MacNodeId nodeId, unsigned int msgId, simtime_t delay, int hops)
{
    if (delay > 0.500)                         // TODO fix this. packets with higher delay should be discarded by the sender
        return;

    // consider the least-significant 16 bits
    unsigned short eventId = (unsigned short)msgId;

    if (eventDeliveryInfo_.find(eventId) == eventDeliveryInfo_.end())
        throw cRuntimeError("d2dMultihopStatistics::recordReception - Event with ID %d does not exist.", eventId);
    if (eventDeliveryInfo_[eventId].find(nodeId) != eventDeliveryInfo_[eventId].end()) {
        if (eventDeliveryInfo_[eventId][nodeId].delay_ < 0) { // store only the minimum
            eventDeliveryInfo_[eventId][nodeId].delay_ = delay;
            eventDeliveryInfo_[eventId][nodeId].hops_ = hops;
        }
    }
}

void MultihopD2DStatistics::recordSentMessage(unsigned int msgId)
{
    // consider the least-significant 16 bits
    unsigned short eventId = (unsigned short)msgId;
    if (eventTransmissionInfo_.find(eventId) == eventTransmissionInfo_.end())
        throw cRuntimeError("d2dMultihopStatistics::recordDuplicateReception - Message with ID %d does not exist.", eventId);
    eventTransmissionInfo_[eventId].numSent_++;
}

void MultihopD2DStatistics::recordSuppressedMessage(unsigned int msgId)
{
    // consider the least-significant 16 bits
    unsigned short eventId = (unsigned short)msgId;
    if (eventTransmissionInfo_.find(eventId) == eventTransmissionInfo_.end())
        throw cRuntimeError("d2dMultihopStatistics::recordDuplicateReception - Message with ID %d does not exist.", eventId);
    eventTransmissionInfo_[eventId].numSuppressed_++;
}

void MultihopD2DStatistics::recordDuplicateReception(unsigned int msgId)
{
    // consider the least-significant 16 bits
    unsigned short eventId = (unsigned short)msgId;
    if (eventTransmissionInfo_.find(eventId) == eventTransmissionInfo_.end())
        throw cRuntimeError("d2dMultihopStatistics::recordDuplicateReception - Message with ID %d does not exist.", eventId);
    eventTransmissionInfo_[eventId].numDuplicates_++;
}

void MultihopD2DStatistics::finish()
{

    // scan structures and emit average statistics
    for (const auto& [eventId, deliveryStatus] : eventDeliveryInfo_) {
        std::vector<simtime_t> sortedDelays;

        unsigned int deliveredMsgCounter = 0;
        simtime_t maxDelay = 0.0;
        for (const auto& [nodeId, receptionStatus] : deliveryStatus) {
            // for each message, insert all delays in this vector
            if (receptionStatus.hops_ >= 0) {
                if (receptionStatus.hops_ >= 1) {
                    sortedDelays.push_back(receptionStatus.delay_);
                    emit(d2dMultihopEventDelaySignal_, receptionStatus.delay_);

                    if (receptionStatus.delay_ > maxDelay)
                        maxDelay = receptionStatus.delay_;
                }
                deliveredMsgCounter++;
            }
        }

        if (sortedDelays.empty())
            continue;

        double deliveryRatio = (double)deliveredMsgCounter / deliveryStatus.size();
        emit(d2dMultihopEventDeliveryRatioSignal_, deliveryRatio);

        // cluster complete covered with a delivery
        int completeDelivery = (deliveredMsgCounter == deliveryStatus.size()) ? 1 : 0;
        emit(d2dMultihopEventCompleteDeliveriesSignal_, completeDelivery);

        // sort the delays and get the percentile you desire
        std::sort(sortedDelays.begin(), sortedDelays.end());
        unsigned int numElements = sortedDelays.size();
        unsigned int index95Percentile = (double)numElements * 0.95;
        emit(d2dMultihopEventDelay95PerSignal_, sortedDelays[index95Percentile]);
    }

    for (const auto& [eventId, transmissionInfo] : eventTransmissionInfo_) {
        emit(d2dMultihopEventSentMsgSignal_, (long)transmissionInfo.numSent_);
        emit(d2dMultihopEventTrickleSuppressedMsgSignal_, (long)transmissionInfo.numSuppressed_);
        emit(d2dMultihopEventRcvdDupMsgSignal_, (long)transmissionInfo.numDuplicates_);
    }
}

} //namespace

