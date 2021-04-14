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

Define_Module(MultihopD2DStatistics);

using namespace omnetpp;

void MultihopD2DStatistics::initialize()
{
    // register statistics
    d2dMultihopEventDeliveryRatio_ = registerSignal("d2dMultihopEventDeliveryRatio");
    d2dMultihopEventDelay_ = registerSignal("d2dMultihopEventDelay");
    d2dMultihopEventDelay95Per_ = registerSignal("d2dMultihopEventDelay95Per");
    d2dMultihopEventSentMsg_ = registerSignal("d2dMultihopEventSentMsg");
    d2dMultihopEventTrickleSuppressedMsg_ = registerSignal("d2dMultihopEventTrickleSuppressedMsg");
    d2dMultihopEventRcvdDupMsg_ = registerSignal("d2dMultihopEventRcvdDupMsg");
    d2dMultihopEventCompleteDeliveries_ = registerSignal("d2dMultihopEventCompleteDeliveries");
}


void MultihopD2DStatistics::recordNewBroadcast(unsigned int msgId, UeSet& destinations)
{
    // consider the least-significant 16 bits
    unsigned short eventId = (unsigned short)msgId;
    if (eventDeliveryInfo_.find(eventId) == eventDeliveryInfo_.end())
    {
        // initialize record for this message
        DeliveryStatus tmp;
        UeSet::iterator it = destinations.begin();
        for (; it != destinations.end(); ++it) {
            ReceptionStatus status;
            status.delay_ = -1.0;
            status.hops_ = -1;
            tmp.insert(std::pair<MacNodeId, ReceptionStatus>(*it,status));
        }
        std::pair<unsigned short, DeliveryStatus> p(eventId, tmp);
        eventDeliveryInfo_.insert(p);
    }
    else
    {
        DeliveryStatus entry = eventDeliveryInfo_.at(eventId);
        UeSet::iterator it = destinations.begin();
        for (; it != destinations.end(); ++it)
        {
            if (eventDeliveryInfo_[eventId].find(*it) == eventDeliveryInfo_[eventId].end())
            {
                // the UE is not in the map
                ReceptionStatus status;
                status.delay_ = -1.0;
                status.hops_ = -1;
                eventDeliveryInfo_[eventId].insert(std::pair<MacNodeId, ReceptionStatus>(*it,status));
            }
        }
    }

    if (eventTransmissionInfo_.find(eventId) == eventTransmissionInfo_.end())
    {
        TransmissionInfo info;
        info.numSent_ = 1;
        eventTransmissionInfo_[eventId] = info;
    }
}


void MultihopD2DStatistics::recordReception(MacNodeId nodeId, unsigned int msgId, omnetpp::simtime_t delay, int hops)
{
    if (delay > 0.500)  // TODO fix this. packets with higher delay should be discarded by the sender
        return;

    // consider the least-significant 16 bits
    unsigned short eventId = (unsigned short)msgId;

    if (eventDeliveryInfo_.find(eventId) == eventDeliveryInfo_.end())
            throw cRuntimeError("d2dMultihopStatistics::recordReception - Event with ID %d does not exist.", eventId);
    if (eventDeliveryInfo_[eventId].find(nodeId) != eventDeliveryInfo_[eventId].end())
    {
        if (eventDeliveryInfo_[eventId][nodeId].delay_ < 0)   // store only the minimum
        {
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
    std::vector<simtime_t> sortedDelays;

    // scan structures and emit average statistics
    std::map<unsigned short, DeliveryStatus>::iterator eit = eventDeliveryInfo_.begin();
    for (; eit != eventDeliveryInfo_.end(); ++eit)
    {
        sortedDelays.clear();

        unsigned int deliveredMsgCounter = 0;
        simtime_t maxDelay = 0.0;
        DeliveryStatus::iterator jt = eit->second.begin();
        for (; jt != eit->second.end(); ++jt)
        {
            // for each message, insert all delays in this vector
            if (jt->second.hops_ >= 0)
            {
                if (jt->second.hops_ >= 1)
                {
                    sortedDelays.push_back(jt->second.delay_);
                    emit(d2dMultihopEventDelay_, jt->second.delay_);

                    if (jt->second.delay_ > maxDelay)
                        maxDelay = jt->second.delay_;
                }
                deliveredMsgCounter++;
            }
        }

        if (sortedDelays.empty())
            continue;

        double deliveryRatio = (double)deliveredMsgCounter / eit->second.size();
        emit(d2dMultihopEventDeliveryRatio_, deliveryRatio);

        // cluster complete covered with a delivery
        int completeDelivery = (deliveredMsgCounter == eit->second.size())? 1 : 0;
        emit(d2dMultihopEventCompleteDeliveries_, completeDelivery);

        // sort the delays and get the percentile you desire
        std::sort(sortedDelays.begin(),sortedDelays.end());
        unsigned int numElements = sortedDelays.size();
        unsigned int index95Percentile = (double)numElements * 0.95;
        emit(d2dMultihopEventDelay95Per_, sortedDelays[index95Percentile]);
    }

    std::map<unsigned short, TransmissionInfo>::iterator eventInfoIt = eventTransmissionInfo_.begin();
    for (; eventInfoIt != eventTransmissionInfo_.end(); ++eventInfoIt)
    {
        emit(d2dMultihopEventSentMsg_, (long)eventInfoIt->second.numSent_);
        emit(d2dMultihopEventTrickleSuppressedMsg_, (long)eventInfoIt->second.numSuppressed_);
        emit(d2dMultihopEventRcvdDupMsg_, (long)eventInfoIt->second.numDuplicates_);
    }
}
