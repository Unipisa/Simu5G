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

#include <climits>
#include "stack/mac/buffer/LteMacQueue.h"
#include "stack/rlc/am/packet/LteRlcAmPdu.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

LteMacQueue::LteMacQueue(int queueSize) :
    cPacketQueue("LteMacQueue"), lastUnenqueueableMainSno(UINT_MAX), queueSize_(queueSize)
{
}

LteMacQueue::LteMacQueue(const LteMacQueue& queue)
{
    operator=(queue);
}

LteMacQueue& LteMacQueue::operator=(const LteMacQueue& queue)
{
    cPacketQueue::operator=(queue);
    queueSize_ = queue.queueSize_;
    return *this;
}

LteMacQueue *LteMacQueue::dup() const
{
    return new LteMacQueue(*this);
}

// ENQUEUE
bool LteMacQueue::pushBack(cPacket *pkt)
{
    Packet *pktAux = check_and_cast<Packet *>(pkt);
    if (!isEnqueueablePacket(pktAux))
        return false; // packet queue full or we have discarded fragments for this main packet

    cPacketQueue::insert(pkt);
    return true;
}

bool LteMacQueue::pushFront(cPacket *pkt)
{
    Packet *pktAux = check_and_cast<Packet *>(pkt);
    if (!isEnqueueablePacket(pktAux))
        return false; // packet queue full or we have discarded fragments for this main packet

    cPacketQueue::insertBefore(cPacketQueue::front(), pkt);
    return true;
}

cPacket *LteMacQueue::popFront()
{
    return getQueueLength() > 0 ? cPacketQueue::pop() : nullptr;
}

cPacket *LteMacQueue::popBack()
{
    return getQueueLength() > 0 ? cPacketQueue::remove(cPacketQueue::back()) : nullptr;
}

simtime_t LteMacQueue::getHolTimestamp() const
{
    return getQueueLength() > 0 ? cPacketQueue::front()->getTimestamp() : 0;
}

int64_t LteMacQueue::getQueueOccupancy() const
{
    return cPacketQueue::getByteLength();
}

int64_t LteMacQueue::getQueueSize() const
{
    return queueSize_;
}

bool LteMacQueue::isEnqueueablePacket(Packet *pkt) {

    auto chunk = pkt->peekAtFront<Chunk>();
    auto pdu = dynamicPtrCast<const LteRlcAmPdu>(chunk);

    if (queueSize_ == 0) {
        // unlimited queue size -- nothing to check for
        return true;
    }
    /* Check:
     *
     * For AM: We need to check if all fragments will fit in the queue
     * For UM: The new UM implementation introduced in commit 9ab9b71c5358a70278e2fbd51bf33a9d1d81cb86
     *         by G. Nardini only sends SDUs upon MAC SDU request. All SDUs are
     *         accepted as long as the MAC queue size is not exceeded.
     * For TM: No fragments are to be checked, anyway.
     */
    if (pdu != nullptr) { // for AM we need to check if all fragments will fit
        if (pdu->getTotalFragments() > 1) {
            int remainingFrags = (pdu->getLastSn() - pdu->getSnoFragment() + 1);
            bool allFragsWillFit = (remainingFrags * pkt->getByteLength()) + getByteLength() < queueSize_;
            bool enqueable = (pdu->getSnoMainPacket() != lastUnenqueueableMainSno) && allFragsWillFit;
            if (allFragsWillFit && !enqueable) {
                EV_DEBUG << "PDU would fit but discarded fragments before - rejecting fragment: " << pdu->getSnoMainPacket() << ":" << pdu->getSnoFragment() << std::endl;
            }
            if (!enqueable) {
                lastUnenqueueableMainSno = pdu->getSnoMainPacket();
            }
            return enqueable;
        }
    }

    // no fragments or unknown type -- can always be enqueued if there is enough space in the queue
    return pkt->getByteLength() + getByteLength() < queueSize_;
}

int LteMacQueue::getQueueLength() const
{
    return cPacketQueue::getLength();
}

std::ostream& operator<<(std::ostream& stream, const LteMacQueue *queue)
{
    stream << "LteMacQueue-> Length: " << queue->getQueueLength() <<
        " Occupancy: " << queue->getQueueOccupancy() <<
        " HolTimestamp: " << queue->getHolTimestamp() <<
        " Size: " << queue->getQueueSize();
    return stream;
}

} //namespace

