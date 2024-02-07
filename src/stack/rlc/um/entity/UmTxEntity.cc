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

#include "stack/rlc/um/entity/UmTxEntity.h"
#include "stack/rlc/am/packet/LteRlcAmPdu.h"

#include "stack/packetFlowManager/PacketFlowManagerUe.h"
#include "stack/packetFlowManager/PacketFlowManagerEnb.h"

Define_Module(UmTxEntity);

using namespace inet;

/*
 * Main functions
 */

void UmTxEntity::initialize()
{
    sno_ = 0;
    firstIsFragment_ = false;
    notifyEmptyBuffer_ = false;
    holdingDownstreamInPackets_ = false;

    // TODO find a more elegant way
    LteMacBase* mac;
    if (strcmp(getParentModule()->getFullName(),"nrRlc") == 0)
        mac = check_and_cast<LteMacBase*>(getParentModule()->getParentModule()->getSubmodule("nrMac"));
    else
        mac = check_and_cast<LteMacBase*>(getParentModule()->getParentModule()->getSubmodule("mac"));

    // store the node id of the owner module
    ownerNodeId_ = mac->getMacNodeId();

    // get the reference to the RLC module
    lteRlc_ = check_and_cast<LteRlcUm*>(getParentModule()->getSubmodule("um"));
    queueSize_ = lteRlc_->par("queueSize");
    queueLength_ = 0;


    // @author Alessandro Noferi
    if(mac->getNodeType() == ENODEB || mac->getNodeType() == GNODEB)
    {
        if(getParentModule()->getParentModule()->findSubmodule("packetFlowManager") != -1)
        {
            EV << "UmTxEntity::initialize - RLC layer if of a base station" << endl;
            packetFlowManager_ = check_and_cast<PacketFlowManagerEnb *>(getParentModule()->getParentModule()->getSubmodule("packetFlowManager"));
        }
    }
    else if(mac->getNodeType() == UE)
    {
        if(strcmp(lteRlc_->getParentModule()->getName(), "nrRlc") == 0)
        {
            if(getParentModule()->getParentModule()->findSubmodule("nrPacketFlowManager") != -1)
            {
                EV << "UmTxEntity::initialize - RLC layer is NRRlc, cast the packetFlowManager to NR" << endl;
                packetFlowManager_ = check_and_cast<PacketFlowManagerUe *>(getParentModule()->getParentModule()->getSubmodule("nrPacketFlowManager"));
            }

        }
        else
        {
            if(getParentModule()->getParentModule()->findSubmodule("packetFlowManager") != -1)
            {
                EV << "UmTxEntity::initialize - RLC layer, cast the packetFlowManager " << endl;
                packetFlowManager_ = check_and_cast<PacketFlowManagerUe *>(getParentModule()->getParentModule()->getSubmodule("packetFlowManager"));
            }
        }
    }

    burstStatus_ = INACTIVE;

}

bool UmTxEntity::enque(cPacket* pkt)
{
    EV << NOW << " UmTxEntity::enque - bufferize new SDU  " << endl;
    if(queueSize_ == 0 || queueLength_ + pkt->getByteLength() < queueSize_){
        // Buffer the SDU in the TX buffer
        sduQueue_.insert(pkt);
        queueLength_ += pkt->getByteLength();
        // Packet was successfully enqueued
        return true;
    } else {
        // Buffer is full - cannot enqueue packet
        return false;
    }
}

void UmTxEntity::rlcPduMake(int pduLength)
{
    EV << NOW << " UmTxEntity::rlcPduMake - PDU with size " << pduLength << " requested from MAC"<< endl;

    // create the RLC PDU
    auto pkt = new inet::Packet("lteRlcFragment");
    auto rlcPdu = inet::makeShared<LteRlcUmDataPdu>();

    // the request from MAC takes into account also the size of the RLC header
    pduLength -= RLC_HEADER_UM;

    int len = 0;

    bool startFrag = firstIsFragment_;
    bool endFrag = false;

    while (!sduQueue_.isEmpty() && pduLength > 0)
    {
        // detach data from the SDU buffer
        cPacket* pktDel = sduQueue_.front();
        auto pkt = check_and_cast<inet::Packet *>(pktDel);
        //auto pkt = check_and_cast<inet::Packet *>(sduQueue_.front());
        auto rlcSdu = pkt->peekAtFront<LteRlcSdu>();
        auto chunk = pkt->peekAtFront<Chunk>();
        //LteRlcSdu rlcSduDel = check_and_cast<LteRlcSdu>(pktDel);
        auto rlcSduDel = dynamicPtrCast<const LteRlcSdu>(chunk);

        if (getSimulation()->getSystemModule()->par("useQosModel").boolValue() && getSimulation()->getSystemModule()->par("packetDropEnabled").boolValue()){
            //check the delay budget of that packet
            //get corresponding delay budget
            LteRlcUm* lteRlc = check_and_cast<LteRlcUm *>(getParentModule()->getSubmodule("um"));
            unsigned short qfi  = flowControlInfo_->getQfi();
            unsigned short _5qi = lteRlc->getQosHandler()->get5Qi(qfi);
            double pdb = lteRlc->getQosHandler()->getPdb(_5qi);

            while (sduQueue_.getLength() > 1){
                double delay = NOW.dbl() - pkt->getCreationTime().dbl();
                if (delay > pdb){
                    //drop packet
                    sduQueue_.pop();
                    delete &rlcSduDel;
                    pkt = check_and_cast<inet::Packet *>(sduQueue_.front());
                    rlcSdu = check_and_cast<LteRlcSdu*>(pkt);
                }
                else {
                    break;
                }
            }


        }


        unsigned int sduSequenceNumber = rlcSdu->getSnoMainPacket();
        int sduLength = rlcSdu->getLengthMainPacket(); // length without the SDU header

        if (fragmentInfo != nullptr) {
            if (fragmentInfo->pkt != pkt)
                throw cRuntimeError("Packets are different");
            sduLength = fragmentInfo->size;
        }

        EV << NOW << " UmTxEntity::rlcPduMake - Next data chunk from the queue, sduSno[" << sduSequenceNumber
                << "], length[" << sduLength << "]"<< endl;

        if (pduLength >= sduLength)
        {
            EV << NOW << " UmTxEntity::rlcPduMake - Add " << sduLength << " bytes to the new SDU, sduSno[" << sduSequenceNumber << "]" << endl;

            // add the whole SDU
            if (fragmentInfo) {
                delete fragmentInfo;
                fragmentInfo = nullptr;
            }
            pduLength -= sduLength;
            len += sduLength;

            pkt = check_and_cast<inet::Packet *>(sduQueue_.pop());
            queueLength_ -= pkt->getByteLength();

            rlcPdu->pushSdu(pkt, sduLength);
            pkt = nullptr;

            EV << NOW << " UmTxEntity::rlcPduMake - Pop data chunk from the queue, sduSno[" << sduSequenceNumber << "]" << endl;

            // now, the first SDU in the buffer is not a fragment
            firstIsFragment_ = false;

            EV << NOW << " UmTxEntity::rlcPduMake - The new SDU has length " << len << ", left space is " << pduLength << endl;
        }
        else
        {
            EV << NOW << " UmTxEntity::rlcPduMake - Add " << pduLength << " bytes to the new SDU, sduSno[" << sduSequenceNumber << "]" << endl;

            // add partial SDU

            len += pduLength;

            auto rlcSduDup = pkt->dup();
            if (fragmentInfo != nullptr) {
                fragmentInfo->size -= pduLength;
                if (fragmentInfo->size < 0)
                    throw cRuntimeError("Fragmentation error");
            }
            else {
                fragmentInfo  = new FragmentInfo;
                fragmentInfo->pkt = pkt;
                fragmentInfo->size = sduLength - pduLength;
            }
            rlcPdu->pushSdu(rlcSduDup, pduLength);

            endFrag = true;

            // update SDU in the buffer
            int newLength = sduLength - pduLength;
            // queueLength_ will be adapted when the whole SDU is removed from the queue

            EV << NOW << " UmTxEntity::rlcPduMake - Data chunk in the queue is now " << newLength << " bytes, sduSno[" << sduSequenceNumber << "]" << endl;

            pduLength = 0;

            // now, the first SDU in the buffer is a fragment
            firstIsFragment_ = true;

            EV << NOW << " UmTxEntity::rlcPduMake - The new SDU has length " << len << ", left space is " << pduLength << endl;

        }
    }

    if (len == 0)
    {
        // send an empty (1-bit) message to notify the MAC that there is not enough space to send RLC PDU
        // (TODO: ugly, should be indicated in a better way)
        EV << NOW << " UmTxEntity::rlcPduMake - cannot send PDU with data, pdulength requested by MAC (" << pduLength << "B) is too small." << std::endl;
        pkt->setName("lteRlcFragment (empty)");
        rlcPdu->setChunkLength(inet::b(1)); // send only a bit, minimum size
    }
    else
    {
        // compute FI
        // the meaning of this field is specified in 3GPP TS 36.322
        FramingInfo fi = 0;
        unsigned short int mask;
        if (endFrag)
        {
            mask = 1;   // 01
            fi |= mask;
        }
        if (startFrag)
        {
            mask = 2;   // 10
            fi |= mask;
        }

        rlcPdu->setFramingInfo(fi);
        rlcPdu->setPduSequenceNumber(sno_++);
        rlcPdu->setChunkLength(inet::B(RLC_HEADER_UM + len));
    }

    *pkt->addTagIfAbsent<FlowControlInfo>() = *flowControlInfo_;


    /*
     * @author Alessandro Noferi
     *
     * Notify the packetFlowManager about the new RLC pdu
     * only in UL or DL cases
     */
    if(flowControlInfo_->getDirection() == DL ||  flowControlInfo_->getDirection() == UL)
    {
        // add RLC PDU to flowpacketmanager
        if(len != 0 && packetFlowManager_ != nullptr)
        {
            LogicalCid lcid = flowControlInfo_->getLcid();

            /* burst management
             *
             * if the buffer is empty, the burst, if ACTIVE,
             * now is finished. Tell the flowmanager to STOP
             * keep trace of burst RLCsm (not the timer). Set burst as INACTIVE
             *
             * if the buffer is NOT empty,
             *      if burst is already ACTIVE, do not start the timer T2
             *      if burst is INACTIVE, START the timer T2 and set it as ACTIVE
             * Tell the flowmanager to keep trace of burst RLCs
             */

            if(sduQueue_.isEmpty())
            {
                if(burstStatus_ == ACTIVE)
                {
                    EV << NOW << " UmTxEntity::burstStatus - ACTIVE -> INACTIVE" << endl;

                    packetFlowManager_->insertRlcPdu(lcid, rlcPdu, STOP);
                    burstStatus_ = INACTIVE;

                }
                else
                {
                    EV << NOW << " UmTxEntity::burstStatus - "<< burstStatus_ << endl;

                    packetFlowManager_->insertRlcPdu(lcid, rlcPdu, burstStatus_);

                }
            }

            else
            {
                if(burstStatus_ == INACTIVE)
                {
                    burstStatus_ = ACTIVE;
                    EV << NOW << " UmTxEntity::burstStatus - INACTIVE -> ACTIVE" << endl;
                    //start a new burst
                    packetFlowManager_->insertRlcPdu(lcid, rlcPdu, START);

                }
                else
                {
                    EV << NOW << " UmTxEntity::burstStatus - burstStatus: "<< burstStatus_ << endl;

                    // burst in still active
                    packetFlowManager_->insertRlcPdu(lcid, rlcPdu, burstStatus_);

                }
            }
        }

    }

    // send to MAC layer
    pkt->insertAtFront(rlcPdu);
    EV << NOW << " UmTxEntity::rlcPduMake - send PDU " << rlcPdu->getPduSequenceNumber() << " with size " << pkt->getByteLength() << " bytes to lower layer" << endl;
    lteRlc_->sendToLowerLayer(pkt);

    // if incoming connection was halted
    if (notifyEmptyBuffer_ && sduQueue_.isEmpty())
    {
        notifyEmptyBuffer_ = false;

        // tell the RLC UM to resume packets for the new mode
        lteRlc_->resumeDownstreamInPackets(flowControlInfo_->getD2dRxPeerId());
    }
}

void UmTxEntity::removeDataFromQueue()
{
    EV << NOW << " UmTxEntity::removeDataFromQueue - removed SDU " << endl;

    // get the last packet...
    cPacket* pkt = sduQueue_.back();

    // ...and remove it
    cPacket* retPkt = sduQueue_.remove(pkt);
    queueLength_ -= retPkt->getByteLength();
    ASSERT(queueLength_ >= 0);
    delete retPkt;
}

void UmTxEntity::clearQueue()
{
    // empty buffer
    while (!sduQueue_.isEmpty())
        delete sduQueue_.pop();

    if (fragmentInfo) {
        delete fragmentInfo;
        fragmentInfo = nullptr;
    }

    queueLength_ = 0;

    // reset variables except for sequence number
    firstIsFragment_ = false;
}

bool UmTxEntity::isHoldingDownstreamInPackets()
{
    return holdingDownstreamInPackets_;
}

void UmTxEntity::enqueHoldingPackets(cPacket* pkt)
{
    EV << NOW << " UmTxEntity::enqueHoldingPackets - storing new SDU into the holding buffer " << endl;
    sduHoldingQueue_.insert(pkt);
}


void UmTxEntity::resumeDownstreamInPackets()
{
    EV << NOW << " UmTxEntity::resumeDownstreamInPackets - resume buffering incoming downstream packets of the RLC entity associated to the new mode" << endl;

    holdingDownstreamInPackets_ = false;

    // move all SDUs in the holding buffer to the TX buffer
    while (!sduHoldingQueue_.isEmpty())
    {
        auto pktRlc = check_and_cast<inet::Packet *> (sduHoldingQueue_.front());
        auto rlcHeader = pktRlc->peekAtFront<LteRlcSdu>();

        sduHoldingQueue_.pop();

        // store the SDU in the TX buffer
        if(enque(pktRlc)){
        	// create a message so as to notify the MAC layer that the queue contains new data
        	auto newDataPkt = inet::makeShared<LteRlcPduNewData>();
        	// make a copy of the RLC SDU
        	auto pktRlcdup = pktRlc->dup();
        	pktRlcdup->insertAtFront(newDataPkt);
            // send the new data indication to the MAC
        	lteRlc_->sendToLowerLayer(pktRlcdup);
        } else {
        	// Queue is full - drop SDU
            EV << "UmTxEntity::resumeDownstreamInPackets - cannot buffer SDU (queue is full), dropping" << std::endl;
            lteRlc_->dropBufferOverflow(pktRlc);
        }
    }
}

void UmTxEntity::rlcHandleD2DModeSwitch(bool oldConnection, bool clearBuffer)
{
    if (oldConnection)
    {
        if (getNodeTypeById(ownerNodeId_) == ENODEB || getNodeTypeById(ownerNodeId_) == GNODEB)
        {
            EV << NOW << " UmRxEntity::rlcHandleD2DModeSwitch - nothing to do on DL leg of IM flow" << endl;
            return;
        }

        if (clearBuffer)
        {
            EV << NOW << " UmTxEntity::rlcHandleD2DModeSwitch - clear TX buffer of the RLC entity associated to the old mode" << endl;
            clearQueue();
        }
        else
        {
            if (!sduQueue_.isEmpty())
            {
                EV << NOW << " UmTxEntity::rlcHandleD2DModeSwitch - check when TX buffer the RLC entity associated to the old mode becomes empty - queue length[" << sduQueue_.getLength() << "]" << endl;
                notifyEmptyBuffer_ = true;
            }
            else
            {
                EV << NOW << " UmTxEntity::rlcHandleD2DModeSwitch - TX buffer of the RLC entity associated to the old mode is already empty" << endl;
            }
        }
    }
    else
    {
        EV << " UmTxEntity::rlcHandleD2DModeSwitch - reset numbering of the RLC TX entity corresponding to the new mode" << endl;
        sno_ = 0;

        if (!clearBuffer)
        {
            if (lteRlc_->isEmptyingTxBuffer(flowControlInfo_->getD2dRxPeerId()))
            {
                // stop incoming connections, until
                EV << NOW << " UmTxEntity::rlcHandleD2DModeSwitch - halt incoming downstream connections of the RLC entity associated to the new mode" << endl;
                startHoldingDownstreamInPackets();
            }
        }
    }
}
