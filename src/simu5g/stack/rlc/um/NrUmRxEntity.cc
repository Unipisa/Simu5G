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

#include "NrUmRxEntity.h"

#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/stack/mac/LteMacEnb.h"

namespace simu5g {

Define_Module(NrUmRxEntity);
using namespace inet;

unsigned int NrUmRxEntity::totalCellPduRcvdBytes_ = 0;
unsigned int NrUmRxEntity::totalCellRcvdBytes_ = 0;

simsignal_t NrUmRxEntity::rlcPacketLossTotalSignal_ = registerSignal("rlcPacketLossTotal");

simsignal_t NrUmRxEntity::rlcCellPacketLossSignal_[2] = { cComponent::registerSignal("rlcCellPacketLossDl"), cComponent::registerSignal("rlcCellPacketLossUl") };
simsignal_t NrUmRxEntity::rlcPacketLossSignal_[2] = { cComponent::registerSignal("rlcPacketLossDl"), cComponent::registerSignal("rlcPacketLossUl") };
simsignal_t NrUmRxEntity::rlcPduPacketLossSignal_[2] = { cComponent::registerSignal("rlcPduPacketLossDl"), cComponent::registerSignal("rlcPduPacketLossUl") };
simsignal_t NrUmRxEntity::rlcDelaySignal_[2] = { cComponent::registerSignal("rlcDelayDl"), cComponent::registerSignal("rlcDelayUl") };
simsignal_t NrUmRxEntity::rlcThroughputSignal_[2] = { cComponent::registerSignal("rlcThroughputDl"), cComponent::registerSignal("rlcThroughputUl") };
simsignal_t NrUmRxEntity::rlcPduDelaySignal_[2] = { cComponent::registerSignal("rlcPduDelayDl"), cComponent::registerSignal("rlcPduDelayUl") };
simsignal_t NrUmRxEntity::rlcPduThroughputSignal_[2] = { cComponent::registerSignal("rlcPduThroughputDl"), cComponent::registerSignal("rlcPduThroughputUl") };
simsignal_t NrUmRxEntity::rlcCellThroughputSignal_[2] = { cComponent::registerSignal("rlcCellThroughputDl"), cComponent::registerSignal("rlcCellThroughputUl") };
simsignal_t NrUmRxEntity::rlcThroughputSampleSignal_[2] ={cComponent::registerSignal("rlcThroughputSampleDl"), cComponent::registerSignal("rlcThroughputSampleUl")};


simsignal_t NrUmRxEntity::rlcPacketLossD2DSignal_ = registerSignal("rlcPacketLossD2D");
simsignal_t NrUmRxEntity::rlcPduPacketLossD2DSignal_ = registerSignal("rlcPduPacketLossD2D");
simsignal_t NrUmRxEntity::rlcDelayD2DSignal_ = registerSignal("rlcDelayD2D");
simsignal_t NrUmRxEntity::rlcThroughputD2DSignal_ = registerSignal("rlcThroughputD2D");
simsignal_t NrUmRxEntity::rlcPduDelayD2DSignal_ = registerSignal("rlcPduDelayD2D");
simsignal_t NrUmRxEntity::rlcPduThroughputD2DSignal_ = registerSignal("rlcPduThroughputD2D");

NrUmRxEntity::NrUmRxEntity()
{
    t_ReassemblyTimer=nullptr;

}

NrUmRxEntity::~NrUmRxEntity()
{
    Enter_Method("~NrUmRxEntity");
    cancelAndDelete(t_ReassemblyTimer);
    delete flowControlInfo_;
}
bool NrUmRxEntity::completeSdu(unsigned int sduSequenceNumber, unsigned int size, bool removeIfComplete)
{
    auto it = sduMap.find(sduSequenceNumber);
    if (it == sduMap.end()) return false;

    auto& segments = it->second;

    // Sort segments based on start offset
    std::sort(segments.begin(), segments.end());

    unsigned int expectedStart = 0;
    unsigned int aux=0;
    for (const auto& seg : segments) {
        EV <<NOW<<"; "<<rlc_<<"; NrAmRxQueue::completeSdu() seg="<<seg.first<<","<<seg.second<<endl;

        if (seg.first != expectedStart) return false;
        aux += (seg.second-seg.first);
        expectedStart = seg.second;

    }
    bool complete=false;
    if (aux==size) {
        complete = true;
        if (removeIfComplete) {
            sduMap.erase(it);
        }
    }
    return complete;


}
void NrUmRxEntity::enque(cPacket *pktAux)
{
    Enter_Method("NrUmRxEntity::enque()");
    take(pktAux);

    EV << NOW << " NrUmRxEntity::enque - buffering new PDU" << endl;

    auto pktPdu = check_and_cast<Packet *>(pktAux);
    auto pdu = pktPdu->removeAtFront<NrRlcUmDataPdu>();
    auto lteInfo = pktPdu->getTag<FlowControlInfo>();
    //TODO: check if supported
    /*
     if (!init_ && isD2DMultiConnection()) {
         // for D2D multicast connections, the first received PDU must be considered as the first valid PDU
         rxWindowDesc_.clear(tsn);
         // setting the window size to 1 allows the entity to deliver immediately out-of-sequence SDU,
         // since reordering is not applicable for D2D multicast communications
         rxWindowDesc_.windowSize_ = 1;
         init_ = true;
     }
     */
    //Check if this is a complete SUD
    if (pdu->getFramingInfo().toValue()==0) {
        //Remove header and deliver  to upper layer
        size_t sduLengthPktLeng;
        auto pktSdu = check_and_cast<Packet *>(pdu->popSdu(sduLengthPktLeng));
        // for burst
        ttiBits_ += sduLengthPktLeng;
        toPdcp(pktSdu);
        pktSdu = nullptr;


    } else {
        // Get the RLC PDU Transmission sequence number (x)
        int tsn = pdu->getPduSequenceNumber();

        if (((RX_Next_Highest-UM_Window_Size)<=tsn) && (tsn<RX_Next_Reassembly)) {
            //Discard: it might be inside the window but RX_Next_Reassembly is the earliest PDU considered for reassembly


        } else {

            handlePDUInReceivedBuffer(pdu,tsn);
        }
    }
    /* @author Alessandro Noferi
     *
     * At the end of each enque, the state of the buffer is checked.
     * A burst end does not mean that actually a burst has occurred,
     * it has occurred if t1 - t2 > TTI
     */

    if (flowControlInfo_->getDirection() == UL) { //only eNodeB checks the burst
        handleBurst(ENQUE);
    }

    // emit statistics
    MacNodeId ueId;
    if (lteInfo->getDirection() == DL || lteInfo->getDirection() == D2D || lteInfo->getDirection() == D2D_MULTI)                                                                                                                    // This module is at a UE
        ueId = ownerNodeId_;
    else           // UL. This module is at the eNB: get the node id of the sender
        ueId = lteInfo->getSourceId();
    totalPduRcvdBytes_ += pktPdu->getByteLength();
    double tputSample = (double)totalPduRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod());

    // emit statistics
    cModule *ue = getRlcByMacNodeId(binder_, ueId, UM);
    if (ue != nullptr) {
        if (lteInfo->getDirection() != D2D && lteInfo->getDirection() != D2D_MULTI) { // UE in IM
            ue->emit(rlcPduThroughputSignal_[dir_], tputSample);
            ue->emit(rlcPduDelaySignal_[dir_], (NOW - pktPdu->getCreationTime()).dbl());
        }
        else { // UE in DM
            ue->emit(rlcPduThroughputD2DSignal_, tputSample);
            ue->emit(rlcPduDelayD2DSignal_, (NOW - pktPdu->getCreationTime()).dbl());
        }
    }
    pktPdu->insertAtFront(pdu);
    delete pktPdu;
    //showStateVariables();

}

void NrUmRxEntity::handlePDUInReceivedBuffer(inet::Ptr<NrRlcUmDataPdu> pdu,  int tsn) {

    //With this we insert the PDU in the buffer actually
    //Notice that since we copy the complete SDU in all the "segmented" PDUs we can always reassemble from the last segment without actually storing the previous ones
    auto& segments = sduMap[tsn];
    //WARNING: this only works if the segments do not change, that is, if we do not resegment the retransmitted SDUs, only retransmit previous segments
    //For UM there is no retransmission anayway
    segments.emplace_back(pdu->getStartOffset(), pdu->getEndOffset());
    // Check if this PDU forms a complete SDU
    if (completeSdu(tsn,pdu->getLengthMainPacket(),true)) { //Delete from map if complete
        size_t sduLengthPktLeng;
        auto pktSdu = check_and_cast<Packet *>(pdu->popSdu(sduLengthPktLeng));
        // for burst
        ttiBits_ += sduLengthPktLeng;

        toPdcp(pktSdu);
        pktSdu = nullptr;


        if (tsn==RX_Next_Reassembly) {
            //We have removed the SN already from the map, so we assign the first next SN
            if (sduMap.empty()) {
                //No SDU waiting for reassembly
                RX_Next_Reassembly=tsn+1;
            } else {
                int first_waiting=sduMap.begin()->first;
                ASSERT(first_waiting>RX_Next_Reassembly);
                RX_Next_Reassembly= first_waiting;

            }

        }
    } else if (!inReassemblyWindow(tsn)){
        RX_Next_Highest=tsn+1;

        //Discard any PDUs that fall outside the reassembly window
        discard(true);

        if (!inReassemblyWindow(RX_Next_Reassembly)) {
            auto it=sduMap.begin();
            while (it!=sduMap.end()) {
                if (it->first>RX_Next_Reassembly) {
                    RX_Next_Reassembly=it->first;
                    break;
                }
                ++it;
            }


        }
    }
    if (t_ReassemblyTimer->isScheduled()) {
        bool condition1=(Rx_Timer_Trigger<=RX_Next_Reassembly);
        bool condition2=(!inReassemblyWindow(Rx_Timer_Trigger) && Rx_Timer_Trigger!=RX_Next_Highest);
        bool condition3=false;
        if (RX_Next_Highest==(RX_Next_Reassembly+1)){
            auto it=sduMap.find(RX_Next_Reassembly);
            if (it==sduMap.end()) {
                condition3=true;
            }
        }
        if (condition1 || condition2 || condition3) {
            cancelEvent(t_ReassemblyTimer);
        }

    }

    //No else here, because the timer might have been cancelled in the previous if
    if (!t_ReassemblyTimer->isScheduled()) {
        //Check and restart if necessary
        restartReassemblyTimer();
    }


}


//void NrUmRxEntity::toPdcp(LteRlcSdu* rlcSdu)
void NrUmRxEntity::toPdcp(Packet *pktAux)
{

    auto rlcSdu = pktAux->popAtFront<LteRlcSdu>();
    NrRlcUm *lteRlc = this->rlc_;

    auto lteInfo = pktAux->getTag<FlowControlInfo>();
    unsigned int sno = rlcSdu->getSnoMainPacket();
    unsigned int length = pktAux->getByteLength();
    simtime_t ts = pktAux->getCreationTime();

    // create a PDCP PDU and send it to the upper layer
    MacNodeId ueId;
    if (lteInfo->getDirection() == DL || lteInfo->getDirection() == D2D || lteInfo->getDirection() == D2D_MULTI)                                                                                                                    // This module is at a UE
        ueId = ownerNodeId_;
    else           // UL. This module is at the eNB: get the node id of the sender
        ueId = lteInfo->getSourceId();

    cModule *ue = getRlcByMacNodeId(binder_, ueId, UM);

    // update the last sno delivered to the current sno
    lastSnoDelivered_ = sno;

    // emit statistics

    totalCellRcvdBytes_ += length;
    totalRcvdBytes_ += length;
    double cellTputSample = (double)totalCellRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod());
    double tputSample = (double)totalRcvdBytes_ / (NOW - getSimulation()->getWarmupPeriod());
    double interval=(NOW-lastTputSample).dbl();
    tpsample  +=length;

    if (ue != nullptr) {
        if (lteInfo->getDirection() != D2D && lteInfo->getDirection() != D2D_MULTI) { // UE in IM
            ue->emit(rlcThroughputSignal_[dir_], tputSample);
            ue->emit(rlcPacketLossSignal_[dir_], 0.0);
            ue->emit(rlcPacketLossTotalSignal_, 0.0);
            ue->emit(rlcDelaySignal_[dir_], (NOW - ts).dbl());
            if (interval>=1) {
                double tput = (double) tpsample/(interval);
                ue->emit(rlcThroughputSampleSignal_[dir_],tput);
                tpsample=0;
                lastTputSample=NOW;
            }

        }
        else {
            ue->emit(rlcThroughputD2DSignal_, tputSample);
            ue->emit(rlcPacketLossD2DSignal_, 0.0);
            ue->emit(rlcPacketLossTotalSignal_, 0.0);
            ue->emit(rlcDelayD2DSignal_, (NOW - ts).dbl());
        }
    }
    if (nodeB_ != nullptr) {
        nodeB_->emit(rlcCellThroughputSignal_[dir_], cellTputSample);
        nodeB_->emit(rlcCellPacketLossSignal_[dir_], 0.0);
    }

    EV << NOW << " NrUmRxEntity::toPdcp Created PDCP PDU with length " << pktAux->getByteLength() << " bytes" << endl;
    EV << NOW << " NrUmRxEntity::toPdcp Send packet to upper layer" << endl;

    lteRlc->sendDefragmented(pktAux);
}


/*
 * Main Functions
 */

void NrUmRxEntity::initialize()
{
    binder_.reference(this, "binderModule", true);

    RX_Next_Highest=0;
    RX_Next_Reassembly=0;
    UM_Window_Size= par("UM_Window_Size");
    Rx_Timer_Trigger=0;

    t_ReassemblyTimer = new cMessage("UM Reassembly timer");
    t_Reassembly= par ("t_Reassembly");

    totalRcvdBytes_ = 0;
    totalPduRcvdBytes_ = 0;

    rlc_.reference(this, "umModule", true);

    //statistics

    LteMacBase *mac = getModuleFromPar<LteMacBase>(par("macModule"), this);
    nodeB_ = getRlcByMacNodeId(binder_, mac->getMacCellId(), UM);

    resetFlag_ = false;

    if (mac->getNodeType() == ENODEB || mac->getNodeType() == GNODEB) {
        dir_ = UL;
        name_entity = "GNB- ";
    }
    else { // UE
        dir_ = DL;
        name_entity = "UE- ";
    }

    // store the node id of the owner module (useful for statistics)
    ownerNodeId_ = mac->getMacNodeId();

    tpsample=0;
    lastTputSample=NOW;

}

void NrUmRxEntity::handleMessage(cMessage *msg)
{
    if (msg==t_ReassemblyTimer) {
        for (const auto& it: sduMap) {
            if (it.first>=Rx_Timer_Trigger) {
                RX_Next_Reassembly=it.first;
                break;
            }
        }
        discard(false); //Discard all SN below RX_Next_Reassembly
        //Check and restart if necessary
        restartReassemblyTimer();
       // showStateVariables();
    }
}
void NrUmRxEntity::discard(bool notInWindow) {
    //notInWindow Discarded because SN is outside the reassembly window or because it is below the next reassemnly
    auto it=sduMap.begin();
    while (it!=sduMap.end()) {
        if (notInWindow) {
            int snpdu=it->first;
            if (!inReassemblyWindow(snpdu)) {
                //Discarding
                // TODO: emit statistics: but we can discard when the timer expires and we need to know which mode D2D? we are
                //rlc->emit(rlcPduPacketLossSignal_[dir_], 1.0);
                it=sduMap.erase(it);
            } else {
                ++it;
            }
        } else {
            if (it->first<RX_Next_Reassembly) {
                // TODO: emit statistics: but we can discard when the timer expires and we need to know which mode D2D? we are
                //rlc->emit(rlcPduPacketLossSignal_[dir_], 1.0);
                it=sduMap.erase(it);
            } else {
                ++it;
            }
        }
    }
}


void NrUmRxEntity::rlcHandleD2DModeSwitch(bool oldConnection, bool oldMode, bool clearBuffer)
{
    Enter_Method_Silent("rlcHandleD2DModeSwitch()");

    if (oldConnection) {
        if (getNodeTypeById(ownerNodeId_) == UE && oldMode == IM) {
            EV << NOW << " NrUmRxEntity::rlcHandleD2DModeSwitch - nothing to do on DL leg of IM flow" << endl;
            return;
        }

        if (clearBuffer) {

            EV << NOW << " NrUmRxEntity::rlcHandleD2DModeSwitch - clear RX buffer of the RLC entity associated with the old mode" << endl;
            //TODO: clear buffer of the entity
            /*for (unsigned int i = 0; i < rxWindowDesc_.windowSize_; i++) {
                // try to reassemble
                reassemble(i);
            }

            // clear the buffer
            pduBuffer_.clear();

            for (auto && i : received_) {
                i = false;
            }

            clearBufferedSdu();

            // stop the timer
            if (t_reordering_.busy())
                t_reordering_.stop();
             */
        }
    }
    else {
        EV << NOW << " NrUmRxEntity::rlcHandleD2DModeSwitch - handle numbering of the RLC entity associated with the newly selected mode" << endl;

        // TODO: reset sequence numbering
        //rxWindowDesc_.clear();

        resetFlag_ = true;
    }
}

void NrUmRxEntity::handleBurst(BurstCheck event)
{
    /* burst management
     * According to ETSI TS 128 552 v15.00.0 and
     * ETSI 136 314 v15.1.0
     * check buffer size:
     *   - if 0:
     *      if(burst = true) //it is ended
     *           consider only total bytes, last_t1 and t2 ( a burst occurred only if t1-t2> TTI
     *           clear variables
     *           reset isBurst
     *      if burst = false nothing happens
     *          clear temp var
     *  - if 1:
     *      if burst = true //burst is still running
     *          update total var with temp value
     *      if burst = false // new burst activated
     *          burst = 1
     *          update total var with temp var
     */
    EV_FATAL << "NrUmRxEntity::handleBurst - size: " << sduMap.size() << endl;

    simtime_t t1 = simTime();

    if (sduMap.empty()) { // last TTI emptied the burst
        if (isBurst_) { // burst ends
            // send stats
            // if the transmission requires two TTIs and I do not count
            // the second last, in the simulator t1 - t2 is 0.

            if ((t1_ - t2_) > TTI) {
                Throughput throughput = { totalBits_, (t1_ - t2_) };
                rlc_->addUeThroughput(flowControlInfo_->getSourceId(), throughput);

                EV_FATAL << "BURST ENDED - size : " << totalBits_ << endl;
                EV_FATAL << "throughput: size " << totalBits_ << " time " << (t1_ - t2_) << endl;
            }
            totalBits_ = 0;
            t2_ = 0;
            t1_ = 0;

            isBurst_ = false;
        }
        else {
            EV_FATAL << "NO BURST - size : " << totalBits_ << endl;
        }
    }
    else {
        if (isBurst_) {
            if (event == ENQUE) { // handleBurst called at the end of the TTI
                totalBits_ += ttiBits_;
                t1_ = t1;
                EV_FATAL << "BURST CONTINUES - size : " << totalBits_ << endl;
            }
        }
        else {
            isBurst_ = true;
            totalBits_ = ttiBits_;
            t2_ = t1;
            t1_ = t1; // it will be updated
            EV_FATAL << "BURST STARTED - size : " << totalBits_ << endl;
        }
    }

    // reset temporary (per TTI) variables
    ttiBits_ = 0;
}

bool NrUmRxEntity::inReassemblyWindow(int sn) {

    if (((RX_Next_Highest-UM_Window_Size)<=sn) && (sn<RX_Next_Highest)) {
        return true;
    } else {
        return false;
    }
}

bool NrUmRxEntity::restartReassemblyTimer() {

    if (RX_Next_Highest>(RX_Next_Reassembly+1) || (RX_Next_Highest==RX_Next_Reassembly+1 && (sduMap.find(RX_Next_Reassembly)!=sduMap.end())) ) {
        scheduleAfter(t_Reassembly, t_ReassemblyTimer);
        Rx_Timer_Trigger=RX_Next_Highest;
        return true;

    } else {
        return false;
    }
}

void NrUmRxEntity::showStateVariables() {
    std::cout<<NOW<<"| RX_Next_Highest |  Lower window | RX_Next_Reassembly | Rx_Timer_Trigger | t_ReassemblyTimer"<<endl;
    std::cout<<NOW<<"| "<< RX_Next_Highest<< " | "<<  (RX_Next_Highest- UM_Window_Size)<<" | " <<RX_Next_Reassembly <<" | "<< Rx_Timer_Trigger<<" | " <<t_ReassemblyTimer->isScheduled()<<endl;
}

} //namespace
