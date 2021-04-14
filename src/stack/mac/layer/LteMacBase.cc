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

#include "common/LteControlInfo.h"
#include "common/binder/Binder.h"
#include "common/cellInfo/CellInfo.h"
#include "stack/mac/layer/LteMacBase.h"
#include "stack/mac/buffer/harq/LteHarqBufferTx.h"
#include "stack/mac/buffer/harq_d2d/LteHarqBufferRxD2D.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/packet/LteMacPdu.h"
#include "stack/mac/buffer/LteMacQueue.h"
#include "stack/mac/packet/LteHarqFeedback_m.h"
#include "stack/mac/packet/LteMacPdu.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "assert.h"

using namespace omnetpp;

LteMacBase::LteMacBase()
{
    mbuf_.clear();
    macBuffers_.clear();
    ttiPeriod_ = TTI;
}

LteMacBase::~LteMacBase()
{
    LteMacBuffers::iterator mit;
    LteMacBufferMap::iterator vit;
    for (mit = mbuf_.begin(); mit != mbuf_.end(); mit++)
        delete mit->second;
    for (vit = macBuffers_.begin(); vit != macBuffers_.end(); vit++)
        delete vit->second;
    mbuf_.clear();
    macBuffers_.clear();

    std::map<double, HarqTxBuffers>::iterator mtit;
    std::map<double, HarqRxBuffers>::iterator mrit;
    HarqTxBuffers::iterator htit;
    HarqRxBuffers::iterator hrit;

    for (mtit = harqTxBuffers_.begin(); mtit != harqTxBuffers_.end(); ++mtit)
        for (htit = mtit->second.begin(); htit != mtit->second.end(); ++htit)
            delete htit->second;

    for (mrit = harqRxBuffers_.begin(); mrit != harqRxBuffers_.end(); ++mrit)
        for (hrit = mrit->second.begin(); hrit != mrit->second.end(); ++hrit)
            delete hrit->second;

    harqTxBuffers_.clear();
    harqRxBuffers_.clear();
}

void LteMacBase::sendUpperPackets(cPacket* pkt)
{
    EV << NOW << " LteMacBase::sendUpperPackets, Sending packet " << pkt->getName() << " on port MAC_to_RLC\n";
    // Send message
    send(pkt,up_[OUT_GATE]);
    nrToUpper_++;
    emit(sentPacketToUpperLayer, pkt);
}

void LteMacBase::sendLowerPackets(cPacket* pkt)
{
    EV << NOW << "LteMacBase::sendLowerPackets, Sending packet " << pkt->getName() << " on port MAC_to_PHY\n";
    // Send message
    updateUserTxParam(pkt);
    send(pkt,down_[OUT_GATE]);
    nrToLower_++;
    emit(sentPacketToLowerLayer, pkt);
}

/*
 * Ue with nodeId left the simulation. Ensure that no
 * signales will be emitted via the deleted node.
 */
void LteMacBase::unregisterHarqBufferRx(MacNodeId nodeId){

    auto hit = harqRxBuffers_.begin();
    for (; hit != harqRxBuffers_.end(); ++hit)
    {
        auto harqRxBuffers = hit->second;
        auto it = harqRxBuffers.find(nodeId);
        if (it != harqRxBuffers.end()){
            it->second->unregister_macUe();
        }
    }
}

/*
 * Upper layer handler
 */
void LteMacBase::fromRlc(cPacket *pkt)
{
    handleUpperMessage(pkt);
}

/*
 * Lower layer handler
 */
void LteMacBase::fromPhy(cPacket *pktAux)
{
    // TODO: harq test (comment fromPhy: it has only to pass pdus to proper rx buffer and
    // to manage H-ARQ feedback)

    auto pkt = check_and_cast<inet::Packet*> (pktAux);
    auto userInfo = pkt->getTag<UserControlInfo>();

    MacNodeId src = userInfo->getSourceId();
    double carrierFreq = userInfo->getCarrierFrequency();

    if (userInfo->getFrameType() == HARQPKT)
    {
        if (harqTxBuffers_.find(carrierFreq) == harqTxBuffers_.end())
        {
            HarqTxBuffers newTxBuffs;
            harqTxBuffers_[carrierFreq] = newTxBuffs;
        }

        // H-ARQ feedback, send it to TX buffer of source
        HarqTxBuffers::iterator htit = harqTxBuffers_[carrierFreq].find(src);
        EV << NOW << "Mac::fromPhy: node " << nodeId_ << " Received HARQ Feedback pkt" << endl;
        if (htit == harqTxBuffers_[carrierFreq].end())
        {
            // if a feedback arrives, a tx buffer must exists (unless it is an handover scenario
            // where the harq buffer was deleted but a feedback was in transit)
            // this case must be taken care of

            if (binder_->hasUeHandoverTriggered(nodeId_) || binder_->hasUeHandoverTriggered(src))
                return;

            throw cRuntimeError("Mac::fromPhy(): Received feedback for an unexisting H-ARQ tx buffer");
        }

        auto hfbpkt = pkt->peekAtFront<LteHarqFeedback>();
        htit->second->receiveHarqFeedback(pkt);
    }
    else if (userInfo->getFrameType() == FEEDBACKPKT)
    {
        //Feedback pkt
        EV << NOW << "Mac::fromPhy: node " << nodeId_ << " Received feedback pkt" << endl;
        macHandleFeedbackPkt(pkt);
    }
    else if (userInfo->getFrameType()==GRANTPKT)
    {
        //Scheduling Grant
        EV << NOW << "Mac::fromPhy: node " << nodeId_ << " Received Scheduling Grant pkt" << endl;
        macHandleGrant(pkt);
    }
    else if(userInfo->getFrameType() == DATAPKT)
    {
        // data packet: insert in proper rx buffer
        EV << NOW << "Mac::fromPhy: node " << nodeId_ << " Received DATA packet" << endl;

        auto pduAux = pkt->peekAtFront<LteMacPdu>();
        auto pdu = pkt;
        Codeword cw = userInfo->getCw();

        if (harqRxBuffers_.find(carrierFreq) == harqRxBuffers_.end())
        {
            HarqRxBuffers newRxBuffs;
            harqRxBuffers_[carrierFreq] = newRxBuffs;
        }

        HarqRxBuffers::iterator hrit = harqRxBuffers_[carrierFreq].find(src);
        if (hrit != harqRxBuffers_[carrierFreq].end())
        {
            hrit->second->insertPdu(cw,pdu);
        }
        else
        {
            // FIXME: possible memory leak
            LteHarqBufferRx *hrb;
            if (userInfo->getDirection() == DL || userInfo->getDirection() == UL)
                hrb = new LteHarqBufferRx(ENB_RX_HARQ_PROCESSES, this,src);
            else // D2D
                hrb = new LteHarqBufferRxD2D(ENB_RX_HARQ_PROCESSES, this,src, (userInfo->getDirection() == D2D_MULTI) );

            harqRxBuffers_[carrierFreq][src] = hrb;
            hrb->insertPdu(cw,pdu);
        }
    }
    else if (userInfo->getFrameType() == RACPKT)
    {
        EV << NOW << "Mac::fromPhy: node " << nodeId_ << " Received RAC packet" << endl;
        macHandleRac(pkt);
    }
    else
    {
        throw cRuntimeError("Unknown packet type %d", (int)userInfo->getFrameType());
    }
}

bool LteMacBase::bufferizePacket(cPacket* pkt)
{
    pkt->setTimestamp();        // Add timestamp with current time to packet

    FlowControlInfo* lteInfo = check_and_cast<FlowControlInfo*>(pkt->getControlInfo());

    // obtain the cid from the packet informations
    MacCid cid = ctrlInfoToMacCid(lteInfo);

    // build the virtual packet corresponding to this incoming packet
    PacketInfo vpkt(pkt->getByteLength(), pkt->getTimestamp());

    LteMacBuffers::iterator it = mbuf_.find(cid);
    if (it == mbuf_.end())
    {
        // Queue not found for this cid: create
        LteMacQueue* queue = new LteMacQueue(queueSize_);
        take(queue);
        LteMacBuffer* vqueue = new LteMacBuffer();

        queue->pushBack(pkt);
        vqueue->pushBack(vpkt);
        mbuf_[cid] = queue;
        macBuffers_[cid] = vqueue;

        // make a copy of lte control info and store it to traffic descriptors map
        FlowControlInfo toStore(*lteInfo);
        connDesc_[cid] = toStore;
        // register connection to lcg map.
        LteTrafficClass tClass = (LteTrafficClass) lteInfo->getTraffic();

        lcgMap_.insert(LcgPair(tClass, CidBufferPair(cid, macBuffers_[cid])));

        EV << "LteMacBuffers : Using new buffer on node: " <<
        MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " <<
        queue->getQueueSize() - queue->getByteLength() << "\n";
    }
    else
    {
        // Found
        LteMacQueue* queue = it->second;

        LteMacBuffer* vqueue = NULL;
        LteMacBufferMap::iterator it = macBuffers_.find(cid);
        if (it != macBuffers_.end())
            vqueue = it->second;

        if (!queue->pushBack(pkt))
        {
            totalOverflowedBytes_ += pkt->getByteLength();
            double sample = (double)totalOverflowedBytes_ / (NOW - getSimulation()->getWarmupPeriod());
            if (lteInfo->getDirection()==DL)
            {
                emit(macBufferOverflowDl_,sample);
            }
            else if (lteInfo->getDirection()==UL)
            {
                emit(macBufferOverflowUl_,sample);
            }
            else // D2D
            {
                emit(macBufferOverflowD2D_,sample);
            }

            EV << "LteMacBuffers : Dropped packet: queue" << cid << " is full\n";
            delete pkt;
            return false;
        }

        if (vqueue != NULL)
        {
            vqueue->pushBack(vpkt);

            EV << "LteMacBuffers : Using old buffer on node: " <<
            MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " <<
            queue->getQueueSize() - queue->getByteLength() << "\n";
        }
        else
            throw cRuntimeError("LteMacBase::bufferizePacket - cannot find mac buffer for cid %d", cid);

    }
        /// After bufferization buffers must be synchronized
    assert(mbuf_[cid]->getQueueLength() == macBuffers_[cid]->getQueueLength());
    return true;
}

void LteMacBase::deleteQueues(MacNodeId nodeId)
{
    LteMacBuffers::iterator mit;
    LteMacBufferMap::iterator vit;
    for (mit = mbuf_.begin(); mit != mbuf_.end(); )
    {
        if (MacCidToNodeId(mit->first) == nodeId)
        {
            while (!mit->second->isEmpty())
            {
                cPacket* pkt = mit->second->popFront();
                delete pkt;
            }
            delete mit->second;        // Delete Queue
            mbuf_.erase(mit++);        // Delete Elem
        }
        else
        {
            ++mit;
        }
    }
    for (vit = macBuffers_.begin(); vit != macBuffers_.end(); )
    {
        if (MacCidToNodeId(vit->first) == nodeId)
        {
            while (!vit->second->isEmpty())
                vit->second->popFront();
            delete vit->second;        // Delete Queue
            macBuffers_.erase(vit++);        // Delete Elem
        }
        else
        {
            ++vit;
        }
    }

    // delete H-ARQ buffers
    std::map<double, HarqTxBuffers>::iterator mtit;
    for (mtit = harqTxBuffers_.begin(); mtit != harqTxBuffers_.end(); ++mtit)
    {
        HarqTxBuffers::iterator hit;
        for (hit = mtit->second.begin(); hit != mtit->second.end(); )
        {
            if (hit->first == nodeId)
            {
                delete hit->second; // Delete Queue
                mtit->second.erase(hit++); // Delete Elem
            }
            else
            {
                ++hit;
            }
        }
    }

    std::map<double, HarqRxBuffers>::iterator mrit;
    for (mrit = harqRxBuffers_.begin(); mrit != harqRxBuffers_.end(); ++mrit)
    {
        HarqRxBuffers::iterator hit2;
        for (hit2 = mrit->second.begin(); hit2 != mrit->second.end();)
        {
            if (hit2->first == nodeId)
            {
                delete hit2->second; // Delete Queue
                mrit->second.erase(hit2++); // Delete Elem
            }
            else
            {
                ++hit2;
            }
        }
    }

    // TODO remove traffic descriptor and lcg entry
}

void LteMacBase::decreaseNumerologyPeriodCounter()
{
    std::map<NumerologyIndex, NumerologyPeriodCounter>::iterator it = numerologyPeriodCounter_.begin();
    for (; it != numerologyPeriodCounter_.end(); ++it)
    {
        if (it->second.current == 0) // reset
            it->second.current = it->second.max - 1;
        else
            it->second.current--;
    }
}

/*
 * Main functions
 */
void LteMacBase::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        /* Gates initialization */
        up_[IN_GATE] = gate("RLC_to_MAC");
        up_[OUT_GATE] = gate("MAC_to_RLC");
        down_[IN_GATE] = gate("PHY_to_MAC");
        down_[OUT_GATE] = gate("MAC_to_PHY");

        /* Create buffers */
        queueSize_ = par("queueSize");

        /* Get reference to binder */
        binder_ = getBinder();

        /* Set The MAC MIB */

        muMimo_ = par("muMimo");

        harqProcesses_ = par("harqProcesses");

        /* statistics */
        statDisplay_ = par("statDisplay");

        totalOverflowedBytes_ = 0;
        nrFromUpper_ = 0;
        nrFromLower_ = 0;
        nrToUpper_ = 0;
        nrToLower_ = 0;

        /* register signals */
        macBufferOverflowDl_ = registerSignal("macBufferOverFlowDl");
        macBufferOverflowUl_ = registerSignal("macBufferOverFlowUl");
        if (isD2DCapable())
            macBufferOverflowD2D_ = registerSignal("macBufferOverFlowD2D");
        receivedPacketFromUpperLayer = registerSignal("receivedPacketFromUpperLayer");
        receivedPacketFromLowerLayer = registerSignal("receivedPacketFromLowerLayer");
        sentPacketToUpperLayer = registerSignal("sentPacketToUpperLayer");
        sentPacketToLowerLayer = registerSignal("sentPacketToLowerLayer");

        measuredItbs_ = registerSignal("measuredItbs");
        WATCH(queueSize_);
        WATCH(nodeId_);
        WATCH_MAP(mbuf_);
        WATCH_MAP(macBuffers_);
    }
}

void LteMacBase::handleMessage(cMessage* msg)
{
    if (msg->isSelfMessage())
    {
        handleSelfMessage();
        scheduleAt(NOW + ttiPeriod_, ttiTick_);
        return;
    }

    cPacket* pkt = check_and_cast<cPacket *>(msg);
    EV << "LteMacBase : Received packet " << pkt->getName() <<
    " from port " << pkt->getArrivalGate()->getName() << endl;

    cGate* incoming = pkt->getArrivalGate();

    if (incoming == down_[IN_GATE])
    {
        // message from PHY_to_MAC gate (from lower layer)
        emit(receivedPacketFromLowerLayer, pkt);
        nrFromLower_++;
        fromPhy(pkt);
    }
    else
    {
        // message from RLC_to_MAC gate (from upper layer)
        emit(receivedPacketFromUpperLayer, pkt);
        nrFromUpper_++;
        fromRlc(pkt);
    }
    return;
}

void LteMacBase::finish()
{
}

void LteMacBase::deleteModule(){
    cancelAndDelete(ttiTick_);
    cSimpleModule::deleteModule();
}

void LteMacBase::refreshDisplay() const
{
    if(statDisplay_){
        char buf[80];

        sprintf(buf, "hl: %ld in, %ld out\nll: %ld in, %ld out", nrFromUpper_, nrToUpper_, nrFromLower_, nrToLower_);

        getDisplayString().setTagArg("t", 0, buf);
        getDisplayString().setTagArg("bgtt", 0, "Number of packets in and ouf the higher layer (hl) and the lower layer (ll).");
    }
}
