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

#ifndef _LTE_LTEMACQUEUE_H_
#define _LTE_LTEMACQUEUE_H_

#include <omnetpp.h>

#include <inet/common/packet/Packet.h>

#include "stack/rlc/packet/LteRlcPdu_m.h"

namespace simu5g {

using namespace omnetpp;

/**
 * @class LteMacQueue
 * @brief Queue for MAC SDU packets
 *
 * The Queue registers the following information:
 * - Packets
 * - Occupation (in bytes)
 * - Maximum size
 * - Number of packets
 * - Head Of Line Timestamp
 *
 * The Queue size can be configured, and packets are
 * dropped if stored packets exceed the queue size
 * A size equal to 0 means that the size is infinite.
 *
 */
class LteMacQueue : public cPacketQueue
{
  public:

    /**
     * Constructor creates a new cPacketQueue
     * with configurable maximum size
     */
    LteMacQueue(int queueSize);


    /**
     * Copy Constructor
     */
    LteMacQueue(const LteMacQueue& queue);
    LteMacQueue& operator=(const LteMacQueue& queue);
    LteMacQueue *dup() const override;

    /**
     * pushBack() inserts a new packet in the back
     * of the queue (standard operation) only
     * if there is enough space left
     *
     * @param pkt packet to insert
     * @return false if queue is full,
     *            true on successful insertion
     */
    bool pushBack(cPacket *pkt);

    /**
     * pushFront() inserts a new packet in the front
     * of the queue only if there is enough space left
     *
     * @param pkt packet to insert
     * @return false if queue is full,
     *            true on successful insertion
     */
    bool pushFront(cPacket *pkt);

    /**
     * popFront() extracts a packet from the
     * front of the queue (standard operation).
     *
     * @return nullptr if queue is empty,
     *            pkt on successful operation
     */
    cPacket *popFront();

    /**
     * popBack() extracts a packet from the
     * back of the queue.
     *
     * @return nullptr if queue is empty,
     *            pkt on successful operation
     */
    cPacket *popBack();

    /**
     * getQueueOccupancy() returns the occupancy
     * of the queue (in bytes)
     *
     * @return queue occupancy
     */
    int64_t getQueueOccupancy() const;

    /**
     * getQueueSize() returns the maximum
     * size of the queue (in bytes)
     *
     * @return queue size
     */
    int64_t getQueueSize() const;

    /**
     * getQueueLength() returns the number
     * of packets in the queue
     *
     * @return #packets in queue
     */
    int getQueueLength() const;

    /**
     * getHolTimestamp() returns the timestamp
     * of the Head Of Line (front) packet of the queue
     *
     * @return Hol Timestamp (0 if queue is empty)
     */
    simtime_t getHolTimestamp() const;

    friend std::ostream& operator<<(std::ostream& stream, const LteMacQueue *queue);

  protected:
    /**
     * Check if it makes sense to enqueue this packet.
     *
     * The check is based on the following assumptions:
     * 1) unfragmented PDUs can always be enqueued
     * 2) PDU fragments should be enqueued if
     *    a) no previous fragment of the same PDU was discarded before
     *    b) we have enough space in the queue to hold all remaining fragments of the same packet
     *
     */
    bool isEnqueueablePacket(inet::Packet *pkt);
    unsigned int lastUnenqueueableMainSno; //<seq. number of

  private:
    /// Size of queue
    int queueSize_;
};

} //namespace

#endif

