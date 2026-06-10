//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "NrUmTxEntity.h"
#include "NrRlcUmDataPdu.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"
#include "simu5g/stack/rlc/packet/LteRlcNewDataTag_m.h"

#include "simu5g/stack/packetFlowObserver/PacketFlowObserverUe.h"
#include "simu5g/stack/packetFlowObserver/PacketFlowObserverEnb.h"
#include "simu5g/stack/mac/buffer/LteMacBuffer.h"
namespace simu5g {
using namespace inet;

Define_Module(NrUmTxEntity);

simsignal_t NrUmTxEntity::wastedGrantedBytes=registerSignal("wastedGrantedBytes");
simsignal_t NrUmTxEntity::requestedPDUSizeSignal=registerSignal("requestedPDUSize");
simsignal_t NrUmTxEntity::sentPDUSizeSignal=registerSignal("sentPDUSize");

NrUmTxEntity::~NrUmTxEntity()
{
    // delete fragmentInfo;
    delete flowControlInfo_;
    sduBuffer->clearBuffer();
}
void NrUmTxEntity::initialize()
{


    notifyEmptyBuffer_ = false;
    holdingDownstreamInPackets_ = false;
    sn_FieldLength=par("sn_FieldLength");
    if (sn_FieldLength!=static_cast<unsigned int>(12)) {
        if (sn_FieldLength!=static_cast<unsigned int>(6)) {
            throw cRuntimeError("NrUmTxEntity::initialize() sn_FieldLength=%u, but only 6 or 12 are valid values ",sn_FieldLength);
        }
    }
    sduBuffer = new RlcUmTransmitterBuffer(static_cast<uint32_t>(sn_FieldLength));
    mac = inet::getConnectedModule<LteMacBase>(getParentModule()->gate("RLC_to_MAC"), 0);

    // store the node id of the owner module
    ownerNodeId_ = mac->getMacNodeId();

    // get the reference to the RLC module
    lteRlc_.reference(this, "umModule", true);


    packetFlowObserver_.reference(this, "packetFlowObserverModule", false);

    // @author Alessandro Noferi
    if (mac->getNodeType() == NODEB) {
        if (packetFlowObserver_) {
            EV << "NrUmTxEntity::initialize - RLC layer is for a base station" << endl;
            ASSERT(check_and_cast<PacketFlowObserverEnb *>(packetFlowObserver_.get()));
        }
        name_entity = "GNB- ";
    }
    else if (mac->getNodeType() == UE) {
        if (packetFlowObserver_) {
            EV << "NrUmTxEntity::initialize - RLC layer, casting the packetFlowObserver " << endl;
            ASSERT(check_and_cast<PacketFlowObserverUe *>(packetFlowObserver_.get()));
        }
        name_entity = "UE- ";
    }

    burstStatus_ = INACTIVE;
}

bool NrUmTxEntity::enque(inet::Packet *sdu)
{
    Enter_Method("NrUmTxEntity::enque()"); // Direct Method Call
    take(sdu);
    sduBuffer->addSdu(sdu->getByteLength(), sdu);
    return true;
}

void NrUmTxEntity::rlcPduMake(int pduLength)
{
    Enter_Method("NrUmTxEntity::rlcPduMake");
    EV << NOW << " NrUmTxEntity::rlcPduMake - PDU with size " << pduLength << " requested from MAC" << endl;
    lteRlc_->emit(requestedPDUSizeSignal, pduLength);
    // create the RLC PDU
    auto pkt = new inet::Packet("lteRlcFragment");
    auto rlcPdu = inet::makeShared<NrRlcUmDataPdu>();

    // the request from MAC takes into account also the size of the RLC header
    int size = pduLength - RLC_HEADER_UM;

    int len = 0;


    bool startFragment=false;
    bool endFragment = false;
    bool middleFragment=false;

    unsigned int sduSequenceNumber =0;
    unsigned int lengthMain =0;
    unsigned int startOffset=0;
    unsigned int endOffset=0;

    if (size<=0) {
        // send an empty (1-bit) message to notify the MAC that there is not enough space to send RLC PDU
        // (TODO: ugly, should be indicated in a better way)
        EV << NOW << " NrUmTxEntity::sendPdus( - cannot send PDU with data, pdulength requested by MAC (" << pduLength << "B) is too small." << std::endl;
        pkt->setName("lteRlcFragment (empty)");
        rlcPdu->setChunkLength(inet::b(1)); // send only a bit, minimum size
        pkt->insertAtFront(rlcPdu);
        lteRlc_->sendFragmented(pkt);
        return;
    }
    PendingSegmentUM segment = sduBuffer->getSegmentForGrant(size);
    if (!segment.isValid) {
        EV << NOW << " NrUmTxEntity::sendPdus()  Requested PDU but buffer is empty. sduBuffer.hasData()= " << sduBuffer->getTotalPendingBytes() << " bytes, size="<<size<< endl;
        lteRlc_->emit(wastedGrantedBytes, size);
        delete pkt;
        return;
    }

    auto bufferedSdu = check_and_cast<inet::Packet *>(segment.ptr);
    auto pdcpTag = bufferedSdu->getTag<PdcpTrackingTag>();
    sduSequenceNumber = pdcpTag->getPdcpSequenceNumber();
    int sduLength = pdcpTag->getOriginalPacketLength(); // length without the SDU header
    rlcPdu->pushSdu(bufferedSdu->dup(), sduLength);
    startOffset=segment.start;
    endOffset=segment.end;
    endFragment=segment.isLastSegment;
    if (segment.isLastSegment || segment.isFull) {
        //delete SDU, it has been previously dupped
        delete bufferedSdu;

    }

    // compute SI
    // the meaning of this field is specified in 3GPP TS 36.322 but is different in TS 38.322, called SI, but we reuse the FramingInfo field
    FramingInfo fi; //00 //full sdu

    if (startFragment) {
        fi.lastIsFragment = true;   // 01
    } else if (endFragment) {
        fi.firstIsFragment = true;   // 10
    } else if (middleFragment) {
        fi.firstIsFragment = true;
        fi.lastIsFragment = true; //11
    }
    //Finish PDU
    len=segment.end-segment.start +1;
    // TODO: the actual length depends on whether the packet includes a segment and a SO, we should do as below,
    // but it affects the MAC, which only takes into account RLC_HEADER_UM, so change it when MAC is changed
    /*
       //for single SDU, just one byte
       if (segment.isFull) {
           len -=1; //We substract one byte because RLC_HEADER_UM is 2, but ETSI TS 132 322 6.2.2.3 says it is only 1 byte
       } else {
        if (startOffset>0) {
           if (sn_FieldLength==12) {

                   //The SO should be present, so we need to add two more bits to the header
                   len +=2;
               } else {
                len +=1;

               }
           }


       }
     */
    rlcPdu->setChunkLength(inet::B(RLC_HEADER_UM + len));
    rlcPdu->setSnoMainPacket(sduSequenceNumber);
    rlcPdu->setLengthMainPacket(lengthMain);
    rlcPdu->setStartOffset(startOffset);
    rlcPdu->setEndOffset(endOffset);
    rlcPdu->setFramingInfo(fi);
    //Full SDUs do not need SN
    if (!segment.isFull) {
        rlcPdu->setPduSequenceNumber(segment.sn);
    }


    *pkt->addTagIfAbsent<FlowControlInfo>() = *flowControlInfo_;




    /*
     * @author Alessandro Noferi
     *
     * Notify the packetFlowObserver about the new RLC PDU
     * only in UL or DL cases
     */
    if (flowControlInfo_->getDirection() == DL || flowControlInfo_->getDirection() == UL) {
        // add RLC PDU to packetFlowObserver
        if (len != 0 && packetFlowObserver_ != nullptr) {
            LogicalCid lcid = flowControlInfo_->getLcid();


            /*
             * Burst management.
             *
             * If the buffer is empty, the burst, if ACTIVE,
             * now is finished. Tell the flow manager to STOP
             * keep track of burst RLCs (not the timer). Set burst as INACTIVE
             *
             * if the buffer is NOT empty,
             *      if burst is already ACTIVE, do not start the timer T2
             *      if burst is INACTIVE, START the timer T2 and set it as ACTIVE
             * Tell the flow manager to keep track of burst RLCs
             */

            //To avoid deriving a new packet flow manager we copy data to a LteRlcUmDataPdu
            inet::Ptr<LteRlcUmDataPdu> ltePdu=inet::makeShared<LteRlcUmDataPdu>();
            auto dup= rlcPdu->dup();
            ltePdu->setChunkLength(inet::B(RLC_HEADER_UM + len));
            ltePdu->setFramingInfo(fi);
            //Full SDUs do not need SN but we set it here anyway
            //ltePdu->setPduSequenceNumber(dup->getPduSequenceNumber());
            size_t s;
            ltePdu->pushSdu(dup->popSdu(s));
            if (!sduBuffer->hasData()) {
                if (burstStatus_ == ACTIVE) {
                    EV << NOW << " NrUmTxEntity::burstStatus - ACTIVE -> INACTIVE" << endl;

                    packetFlowObserver_->insertRlcPdu(lcid, ltePdu, STOP);
                    burstStatus_ = INACTIVE;
                }
                else {
                    EV << NOW << " NrUmTxEntity::burstStatus - " << burstStatus_ << endl;

                    packetFlowObserver_->insertRlcPdu(lcid, ltePdu, burstStatus_);
                }
            }
            else {
                if (burstStatus_ == INACTIVE) {
                    burstStatus_ = ACTIVE;
                    EV << NOW << " NrUmTxEntity::burstStatus - INACTIVE -> ACTIVE" << endl;
                    //start a new burst
                    packetFlowObserver_->insertRlcPdu(lcid, ltePdu, START);
                }
                else {
                    EV << NOW << " NrUmTxEntity::burstStatus - burstStatus: " << burstStatus_ << endl;

                    // burst is still active
                    packetFlowObserver_->insertRlcPdu(lcid, ltePdu, burstStatus_);
                }
            }
            delete dup;
            //delete ltePdu;
        }
    }

    // send to MAC layer
    pkt->insertAtFront(rlcPdu);
    EV << NOW << " NrUmTxEntity::rlcPduMake - send PDU " << rlcPdu->getPduSequenceNumber() << " with size " << pkt->getByteLength() << " bytes to lower layer" << endl;
    lteRlc_->emit(sentPDUSizeSignal, pkt->getByteLength());
    lteRlc_->sendToLowerLayer(pkt);

    // if incoming connection was halted
    if (notifyEmptyBuffer_ && !sduBuffer->hasData()) {
        notifyEmptyBuffer_ = false;

        // tell the RLC UM to resume packets for the new mode
        lteRlc_->resumeDownstreamInPackets(flowControlInfo_->getD2dRxPeerId());
    }
    //TODO: workaround, check what the MAC thinks there is in the buffers
    reportBufferStatus(pkt);

}
void NrUmTxEntity::setFlowControlInfo(FlowControlInfo *lteInfo, MacCid cid) {
    flowControlInfo_ = lteInfo->dup();
    infoCid_=cid;
}
void  NrUmTxEntity::reportBufferStatus(inet::Packet* pkt) {


    if (sduBuffer->hasData() ) {
        unsigned int pendingData=sduBuffer->getTotalPendingBytes();

        unsigned int macOccupancy = mac->getMacBuffer(infoCid_)->getQueueOccupancy();

        if (macOccupancy==0 && pendingData>0) {
            // create the RLC PDU
            auto pktInform = new inet::Packet("lteRlcFragment");
            auto rlcPduInform = inet::makeShared<NrRlcUmDataPdu>();
            rlcPduInform->setChunkLength(B(pendingData));
            //pkt->copyTags(*pktAux);
            *(pktInform->addTagIfAbsent<FlowControlInfo>()) = *flowControlInfo_;
            pktInform->insertAtFront(rlcPduInform);
            //pkt->setByteLength(sduLength);
            lteRlc_->indicateNewDataToMac(pkt);
            delete pktInform;
        }
    }

}



bool NrUmTxEntity::isHoldingDownstreamInPackets()
{
    return holdingDownstreamInPackets_;
}

void NrUmTxEntity::enqueHoldingPackets(cPacket *pkt)
{
    EV << NOW << " NrUmTxEntity::enqueHoldingPackets - storing new SDU into the holding buffer " << endl;
    sduHoldingQueue_.insert(pkt);
}

void NrUmTxEntity::resumeDownstreamInPackets()
{
    EV << NOW << " NrUmTxEntity::resumeDownstreamInPackets - resume buffering incoming downstream packets of the RLC entity associated with the new mode" << endl;

    holdingDownstreamInPackets_ = false;

    // move all SDUs in the holding buffer to the TX buffer
    while (!sduHoldingQueue_.isEmpty()) {
        auto pktRlc = check_and_cast<inet::Packet *>(sduHoldingQueue_.front());

        sduHoldingQueue_.pop();

        // store the SDU in the TX buffer
        if (enque(pktRlc)) {
            // notify the MAC layer of new data via tag (replaces LteRlcPduNewData chunk)
            auto pktRlcdup = pktRlc->dup();
            pktRlcdup->addTag<LteRlcNewDataTag>();
            // send the new data indication to the MAC
            lteRlc_->sendToLowerLayer(pktRlcdup);
        }
        else {
            // Queue is full - drop SDU
            EV << "NrUmTxEntity::resumeDownstreamInPackets - cannot buffer SDU (queue is full), dropping" << std::endl;
            lteRlc_->dropBufferOverflow(pktRlc);
        }
    }
}

void NrUmTxEntity::rlcHandleD2DModeSwitch(bool oldConnection, bool clearBuffer)
{
    if (oldConnection) {
        if (getNodeTypeById(ownerNodeId_) == NODEB) {
            EV << NOW << " UmRxEntity::rlcHandleD2DModeSwitch - nothing to do on DL leg of IM flow" << endl;
            return;
        }

        if (clearBuffer) {
            //TODO: clear buffer
            //EV << NOW << " NrUmTxEntity::rlcHandleD2DModeSwitch - clear TX buffer of the RLC entity associated with the old mode" << endl;
            //clearQueue();
            sduBuffer->clearBuffer();
        }
        else {
            if (sduBuffer->hasData()) {
                EV << NOW << " NrUmTxEntity::rlcHandleD2DModeSwitch - check when the TX buffer of the RLC entity associated with the old mode becomes empty - queue length[" << sduBuffer->getTotalPendingBytes() << "]" << endl;
                notifyEmptyBuffer_ = true;
            }
            else {
                EV << NOW << " NrUmTxEntity::rlcHandleD2DModeSwitch - TX buffer of the RLC entity associated with the old mode is already empty" << endl;
            }
        }
    }
    else {
        //TODO:
        EV << " NrUmTxEntity::rlcHandleD2DModeSwitch - reset numbering of the RLC TX entity corresponding to the new mode" << endl;
        sduBuffer->resetTxNext();

        if (!clearBuffer) {
            if (lteRlc_->isEmptyingTxBuffer(flowControlInfo_->getD2dRxPeerId())) {
                // stop incoming connections, until
                EV << NOW << " NrUmTxEntity::rlcHandleD2DModeSwitch - halt incoming downstream connections of the RLC entity associated with the new mode" << endl;
                startHoldingDownstreamInPackets();
            }
        }

    }
}

} //namespace
