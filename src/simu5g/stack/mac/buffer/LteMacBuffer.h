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

#ifndef _LTE_LTEMACBUFFER_H_
#define _LTE_LTEMACBUFFER_H_

#include <omnetpp.h>
#include "common/LteCommon.h"

namespace simu5g {

using namespace omnetpp;

class LteMacQueue;

/**
 * @class LteMacBuffer
 * @brief  Buffers for MAC packets
 */
class LteMacBuffer
{
  public:
    /**
     * Constructor initializes
     * the list
     */
    LteMacBuffer();

    /**
     * Copy Constructors
     */
    LteMacBuffer(const LteMacQueue& queue);
    LteMacBuffer& operator=(const LteMacBuffer& queue);
    LteMacBuffer *dup() const;

    /**
     * pushBack() inserts a new packet
     * at the back of the queue (standard operation)
     *
     * @param pkt packet to insert
     */
    void pushBack(PacketInfo pkt);

    /**
     * pushFront() inserts a new
     * packet at the front of the queue
     *
     * @param pkt packet to insert
     */
    void pushFront(PacketInfo pkt);

    /**
     * popFront() extracts a packet from the
     * front of the queue (standard operation).
     * NOTE: This function increases the processed_ variable.
     *
     * @return zero-size packet if queue is empty,
     *             pkt on successful operation
     */
    PacketInfo popFront();

    /**
     * popBack() extracts a packet from the
     * back of the queue.
     *
     * @return zero-size packet if queue is empty,
     *             pkt on successful operation
     */
    PacketInfo popBack();

    /**
     * front() returns the packet in front
     * of the queue without performing actual extraction.
     *
     * @return zero-size packet if queue is empty,
     *             pkt on successful operation
     */
    PacketInfo& front();

    /**
     * back() returns the packet at the back
     * of the queue without performing actual extraction.
     *
     * @return nullptr if queue is empty,
     *            pointer to pkt on successful operation
     */
    PacketInfo back() const;

    /**
     * setProcessed() sets the value of the
     * processed_ variable
     *
     * @param i value for processed_
     */
    void setProcessed(unsigned int i);

    /**
     * getQueueOccupancy() returns the occupancy
     * of the queue (in bytes).
     *
     * @return queue occupancy
     */
    unsigned int getQueueOccupancy() const;

    /**
     * getQueueLength() returns the number
     * of packets in the queue
     *
     * @return #packets in queue
     */
    int getQueueLength() const;

    /**
     * isEmpty()
     * @return TRUE if the queue is empty
     */
    bool isEmpty() const;

    /**
     * getHolTimestamp() returns the timestamp
     * of the Head Of Line (front)
     * packet of the queue
     *
     * @return Hol Timestamp (0 if queue empty)
     */
    simtime_t getHolTimestamp() const;

    /**
     * getProcessed() returns the number of SDUs
     * processed by the scheduler. It is assumed that
     * the scheduler processes packets only from the front of the queue
     *
     * @return Number of processed SDUs
     */
    unsigned int getProcessed() const;

    /**
     * Get direct (readonly) access to PDU list
     */
    const std::list<PacketInfo> *getPacketlist() const;

    friend std::ostream& operator<<(std::ostream& stream, const LteMacQueue *queue);

  private:
    /// Number of packets processed by the scheduler
    unsigned int processed_;

    /// Occupancy of the whole buffer
    unsigned int queueOccupancy_;

    /// Number of queued packets
    int queueLength_;

    /// List of packets
    std::list<PacketInfo> Queue_;
};

} //namespace

#endif

