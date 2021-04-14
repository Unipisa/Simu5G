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

#include <assert.h>
#include "stack/phy/layer/LtePhyUe.h"

#include "stack/ip2nic/IP2Nic.h"
#include "stack/phy/packet/LteFeedbackPkt.h"
#include "stack/phy/feedback/LteDlFeedbackGenerator.h"

Define_Module(LtePhyUe);

using namespace inet;

LtePhyUe::LtePhyUe()
{
    handoverStarter_ = nullptr;
    handoverTrigger_ = nullptr;
}

LtePhyUe::~LtePhyUe()
{
    cancelAndDelete(handoverStarter_);
    delete das_;
}

void LtePhyUe::initialize(int stage)
{
    LtePhyBase::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL)
    {
        isNr_ = false;        // this might be true only if this module is a NrPhyUe
        nodeType_ = UE;
        useBattery_ = false;  // disabled
        enableHandover_ = par("enableHandover");
        handoverLatency_ = par("handoverLatency").doubleValue();
        handoverDetachment_ = handoverLatency_/2.0;                      // TODO: make this configurable from NED
        handoverAttachment_ = handoverLatency_ - handoverDetachment_;
        dynamicCellAssociation_ = par("dynamicCellAssociation");
        if (par("minRssiDefault").boolValue())
            minRssi_ = binder_->phyPisaData.minSnr();
        else
            minRssi_ = par("minRssi").doubleValue();

        currentMasterRssi_ = 0;
        candidateMasterRssi_ = 0;
        hysteresisTh_ = 0;
        hysteresisFactor_ = 10;
        handoverDelta_ = 0.00001;

        dasRssiThreshold_ = 1.0e-5;
        das_ = new DasFilter(this, binder_, nullptr, dasRssiThreshold_);

        servingCell_ = registerSignal("servingCell");
        averageCqiDl_ = registerSignal("averageCqiDl");
        averageCqiUl_ = registerSignal("averageCqiUl");

        if (!hasListeners(averageCqiDl_))
            error("no phy listeners");

        WATCH(nodeType_);
        WATCH(masterId_);
        WATCH(candidateMasterId_);
        WATCH(dasRssiThreshold_);
        WATCH(currentMasterRssi_);
        WATCH(candidateMasterRssi_);
        WATCH(hysteresisTh_);
        WATCH(hysteresisFactor_);
        WATCH(handoverDelta_);
    }
    else if (stage == inet::INITSTAGE_PHYSICAL_ENVIRONMENT)
    {
        if (useBattery_)
        {
            // TODO register the device to the battery with two accounts, e.g. 0=tx and 1=rx
            // it only affects statistics
            //registerWithBattery("LtePhy", 2);
//            txAmount_ = par("batteryTxCurrentAmount");
//            rxAmount_ = par("batteryRxCurrentAmount");
//
//            WATCH(txAmount_);
//            WATCH(rxAmount_);
        }

        txPower_ = ueTxPower_;

        lastFeedback_ = 0;

        handoverStarter_ = new cMessage("handoverStarter");

        if (isNr_)
        {
            mac_ = check_and_cast<LteMacUe *>(
                getParentModule()-> // nic
                getSubmodule("nrMac"));
            rlcUm_ = check_and_cast<LteRlcUm*>(
                getParentModule()-> // nic
                getSubmodule("nrRlc")->
                    getSubmodule("um"));
        }
        else
        {
            mac_ = check_and_cast<LteMacUe *>(
                getParentModule()-> // nic
                getSubmodule("mac"));
            rlcUm_ = check_and_cast<LteRlcUm*>(
                getParentModule()-> // nic
                getSubmodule("rlc")->
                    getSubmodule("um"));
        }
        pdcp_ = check_and_cast<LtePdcpRrcBase*>(
            getParentModule()-> // nic
            getSubmodule("pdcpRrc"));

        // get local id
        if (isNr_)
            nodeId_ = getAncestorPar("nrMacNodeId");
        else
            nodeId_ = getAncestorPar("macNodeId");
        EV << "Local MacNodeId: " << nodeId_ << endl;
    }
    else if (stage == inet::INITSTAGE_PHYSICAL_LAYER)
    {
        // get serving cell from configuration
        // TODO find a more elegant way
        if (isNr_)
            masterId_ = getAncestorPar("nrMasterId");
        else
            masterId_ = getAncestorPar("masterId");
        candidateMasterId_ = masterId_;

        // find the best candidate master cell
        if (dynamicCellAssociation_)
        {
            // this is a fictitious frame that needs to compute the SINR
            LteAirFrame *frame = new LteAirFrame("cellSelectionFrame");
            UserControlInfo *cInfo = new UserControlInfo();

            // get the list of all eNodeBs in the network
            std::vector<EnbInfo*>* enbList = binder_->getEnbList();
            std::vector<EnbInfo*>::iterator it = enbList->begin();
            for (; it != enbList->end(); ++it)
            {
                // the NR phy layer only checks signal from gNBs
                if (isNr_ && (*it)->nodeType != GNODEB)
                    continue;

                // the LTE phy layer only checks signal from eNBs
                if (!isNr_ && (*it)->nodeType != ENODEB)
                    continue;

                MacNodeId cellId = (*it)->id;
                LtePhyBase* cellPhy = check_and_cast<LtePhyBase*>((*it)->eNodeB->getSubmodule("cellularNic")->getSubmodule("phy"));
                double cellTxPower = cellPhy->getTxPwr();
                Coord cellPos = cellPhy->getCoord();

                // TODO need to check if the eNodeB uses the same carrier frequency as the UE

                // build a control info
                cInfo->setSourceId(cellId);
                cInfo->setTxPower(cellTxPower);
                cInfo->setCoord(cellPos);
                cInfo->setFrameType(FEEDBACKPKT);

                // get RSSI from the eNB
                std::vector<double>::iterator it;
                double rssi = 0;
                std::vector<double> rssiV = primaryChannelModel_->getSINR(frame, cInfo);
                for (it = rssiV.begin(); it != rssiV.end(); ++it)
                    rssi += *it;
                rssi /= rssiV.size();   // compute the mean over all RBs

                EV << "LtePhyUe::initialize - RSSI from eNodeB " << cellId << ": " << rssi << " dB (current candidate eNodeB " << candidateMasterId_ << ": " << candidateMasterRssi_ << " dB" << endl;

                if (rssi > candidateMasterRssi_)
                {
                    candidateMasterId_ = cellId;
                    candidateMasterRssi_ = rssi;
                }
            }
            delete cInfo;
            delete frame;

            // binder calls
            // if dynamicCellAssociation selected a different master
            if (candidateMasterId_ != 0 && candidateMasterId_ != masterId_)
            {
                binder_->unregisterNextHop(masterId_, nodeId_);
                binder_->registerNextHop(candidateMasterId_, nodeId_);
            }
            masterId_ = candidateMasterId_;
            // set serving cell
            if (isNr_)
                getAncestorPar("nrMasterId").setIntValue(masterId_);
            else
                getAncestorPar("masterId").setIntValue(masterId_);
            currentMasterRssi_ = candidateMasterRssi_;
            updateHysteresisTh(candidateMasterRssi_);
        }

        EV << "LtePhyUe::initialize - Attaching to eNodeB " << masterId_ << endl;

        das_->setMasterRuSet(masterId_);
        emit(servingCell_, (long)masterId_);
    }
    else if (stage == inet::INITSTAGE_NETWORK_CONFIGURATION)
    {
        // get cellInfo at this stage because the next hop of the node is registered in the IP2Nic module at the INITSTAGE_NETWORK_LAYER
        if (masterId_ > 0)
        {
            cellInfo_ = getCellInfo(nodeId_);
            int index = intuniform(0, binder_->phyPisaData.maxChannel() - 1);
            if (cellInfo_ != NULL)
            {
                cellInfo_->lambdaInit(nodeId_, index);
                cellInfo_->channelUpdate(nodeId_, intuniform(1, binder_->phyPisaData.maxChannel2()));
            }
        }
        else
            cellInfo_ = NULL;
    }
}

void LtePhyUe::handleSelfMessage(cMessage *msg)
{
    if (msg->isName("handoverStarter"))
        triggerHandover();
    else if (msg->isName("handoverTrigger"))
    {
        doHandover();
        delete msg;
        handoverTrigger_ = nullptr;
    }
}

void LtePhyUe::handoverHandler(LteAirFrame* frame, UserControlInfo* lteInfo)
{
    lteInfo->setDestId(nodeId_);
    if (!enableHandover_)
    {
        // Even if handover is not enabled, this call is necessary
        // to allow Reporting Set computation.
        if (getNodeTypeById(lteInfo->getSourceId()) == ENODEB && lteInfo->getSourceId() == masterId_)
        {
            // Broadcast message from my master enb
            das_->receiveBroadcast(frame, lteInfo);
        }

        delete frame;
        delete lteInfo;
        return;
    }

    frame->setControlInfo(lteInfo);
    double rssi;

    if (getNodeTypeById(lteInfo->getSourceId()) == ENODEB && lteInfo->getSourceId() == masterId_)
    {
        // Broadcast message from my master enb
        rssi = das_->receiveBroadcast(frame, lteInfo);
    }
    else
    {
        // Broadcast message from not-master enb
        std::vector<double>::iterator it;
        rssi = 0;
        std::vector<double> rssiV = primaryChannelModel_->getSINR(frame, lteInfo);
        for (it = rssiV.begin(); it != rssiV.end(); ++it)
            rssi += *it;
        rssi /= rssiV.size();
    }

    EV << "UE " << nodeId_ << " broadcast frame from " << lteInfo->getSourceId() << " with RSSI: " << rssi << " at " << simTime() << endl;

    if (lteInfo->getSourceId() != masterId_ && rssi < minRssi_)
    {
        EV << "Signal too weak from a candidate master - minRssi[" << minRssi_ << "]" << endl;
        delete frame;
        return;
    }


    if (rssi > candidateMasterRssi_ + hysteresisTh_)
    {
        if (lteInfo->getSourceId() == masterId_)
        {
            // receiving even stronger broadcast from current master
            currentMasterRssi_ = rssi;
            candidateMasterId_ = masterId_;
            candidateMasterRssi_ = rssi;
            hysteresisTh_ = updateHysteresisTh(currentMasterRssi_);
            cancelEvent(handoverStarter_);
        }
        else
        {
            // broadcast from another master with higher rssi
            candidateMasterId_ = lteInfo->getSourceId();
            candidateMasterRssi_ = rssi;
            hysteresisTh_ = updateHysteresisTh(rssi);
            binder_->addHandoverTriggered(nodeId_, masterId_, candidateMasterId_);

            // schedule self message to evaluate handover parameters after
            // all broadcast messages are arrived
            if (!handoverStarter_->isScheduled())
            {
                // all broadcast messages are scheduled at the very same time, a small delta
                // guarantees the ones belonging to the same turn have been received
                scheduleAt(simTime() + handoverDelta_, handoverStarter_);
            }
        }
    }
    else
    {
        if (lteInfo->getSourceId() == masterId_)
        {
            if (rssi >= minRssi_)
            {
                currentMasterRssi_ = rssi;
                candidateMasterRssi_ = rssi;
                hysteresisTh_ = updateHysteresisTh(rssi);
            }
            else  // lost connection from current master
            {
                if (candidateMasterId_ == masterId_)  // trigger detachment
                {
                    candidateMasterId_ = 0;
                    candidateMasterRssi_ = 0;
                    hysteresisTh_ = updateHysteresisTh(0);
                    binder_->addHandoverTriggered(nodeId_, masterId_, candidateMasterId_);

                    if (!handoverStarter_->isScheduled())
                    {
                        // all broadcast messages are scheduled at the very same time, a small delta
                        // guarantees the ones belonging to the same turn have been received
                        scheduleAt(simTime() + handoverDelta_, handoverStarter_);
                    }
                }
                // else do nothing, a stronger RSSI from another nodeB has been found already
            }
        }
    }

    delete frame;
}

void LtePhyUe::triggerHandover()
{
    ASSERT(masterId_ != candidateMasterId_);

    EV << "####Handover starting:####" << endl;
    EV << "current master: " << masterId_ << endl;
    EV << "current rssi: " << currentMasterRssi_ << endl;
    EV << "candidate master: " << candidateMasterId_ << endl;
    EV << "candidate rssi: " << candidateMasterRssi_ << endl;
    EV << "############" << endl;

    if (candidateMasterRssi_ == 0)
        EV << NOW << " LtePhyUe::triggerHandover - UE " << nodeId_ << " lost its connection to eNB " << masterId_ << ". Now detaching... " << endl;
    else if (masterId_ == 0)
        EV << NOW << " LtePhyUe::triggerHandover - UE " << nodeId_ << " is starting attachment procedure to eNB " << candidateMasterId_ << "... " << endl;
    else
        EV << NOW << " LtePhyUe::triggerHandover - UE " << nodeId_ << " is starting handover to eNB " << candidateMasterId_ << "... " << endl;

    binder_->addUeHandoverTriggered(nodeId_);

    // inform the UE's IP2Nic module to start holding downstream packets
    IP2Nic* ip2nic =  check_and_cast<IP2Nic*>(getParentModule()->getSubmodule("ip2nic"));
    ip2nic->triggerHandoverUe(candidateMasterId_);
    binder_->removeHandoverTriggered(nodeId_);

    // inform the eNB's IP2Nic module to forward data to the target eNB
    if (masterId_ != 0 && candidateMasterId_ != 0)
    {
        IP2Nic* enbIp2Nic =  check_and_cast<IP2Nic*>(getSimulation()->getModule(binder_->getOmnetId(masterId_))->getSubmodule("cellularNic")->getSubmodule("ip2nic"));
        enbIp2Nic->triggerHandoverSource(nodeId_,candidateMasterId_);
    }

    double handoverLatency;
    if (masterId_ == 0)                        // attachment only
        handoverLatency = handoverAttachment_;
    else if (candidateMasterId_ == 0)          // detachment only
        handoverLatency = handoverDetachment_;
    else
        handoverLatency = handoverDetachment_ + handoverAttachment_;

    handoverTrigger_ = new cMessage("handoverTrigger");
    scheduleAt(simTime() + handoverLatency, handoverTrigger_);
}

void LtePhyUe::doHandover()
{
    // if masterId_ == 0, it means the UE was not attached to any eNodeB, so it only has to perform attachment procedures
    // if candidateMasterId_ == 0, it means the UE is detaching from its eNodeB, so it only has to perform detachment procedures

    if (masterId_ != 0)
    {
        // Delete Old Buffers
        deleteOldBuffers(masterId_);

        // amc calls
        LteAmc *oldAmc = getAmcModule(masterId_);
        oldAmc->detachUser(nodeId_, UL);
        oldAmc->detachUser(nodeId_, DL);
    }

    if (candidateMasterId_ != 0)
    {
        LteAmc *newAmc = getAmcModule(candidateMasterId_);
        assert(newAmc != nullptr);
        newAmc->attachUser(nodeId_, UL);
        newAmc->attachUser(nodeId_, DL);
    }

    // binder calls
    if (masterId_ != 0)
        binder_->unregisterNextHop(masterId_, nodeId_);

    if (candidateMasterId_ != 0)
    {
        binder_->registerNextHop(candidateMasterId_, nodeId_);
        das_->setMasterRuSet(candidateMasterId_);
    }
    binder_->updateUeInfoCellId(nodeId_,candidateMasterId_);

    // change masterId and notify handover to the MAC layer
    MacNodeId oldMaster = masterId_;
    masterId_ = candidateMasterId_;
    mac_->doHandover(candidateMasterId_);  // do MAC operations for handover
    currentMasterRssi_ = candidateMasterRssi_;
    hysteresisTh_ = updateHysteresisTh(currentMasterRssi_);

    // update cellInfo
    if (masterId_ != 0)
        cellInfo_->detachUser(nodeId_);

    if (candidateMasterId_ != 0)
    {
        CellInfo* oldCellInfo = cellInfo_;
        LteMacEnb* newMacEnb =  check_and_cast<LteMacEnb*>(getSimulation()->getModule(binder_->getOmnetId(candidateMasterId_))->getSubmodule("cellularNic")->getSubmodule("mac"));
        CellInfo* newCellInfo = newMacEnb->getCellInfo();
        newCellInfo->attachUser(nodeId_);
        cellInfo_ = newCellInfo;
        if (oldCellInfo == NULL)
        {
            // first time the UE is attached to someone
            int index = intuniform(0, binder_->phyPisaData.maxChannel() - 1);
            cellInfo_->lambdaInit(nodeId_, index);
            cellInfo_->channelUpdate(nodeId_, intuniform(1, binder_->phyPisaData.maxChannel2()));
        }
    }

    // update DL feedback generator
    LteDlFeedbackGenerator* fbGen;
    fbGen = check_and_cast<LteDlFeedbackGenerator*>(getParentModule()->getSubmodule("dlFbGen"));
    fbGen->handleHandover(masterId_);

    // collect stat
    emit(servingCell_, (long)masterId_);

    if (masterId_ == 0)
        EV << NOW << " LtePhyUe::doHandover - UE " << nodeId_ << " detached from the network" << endl;
    else
        EV << NOW << " LtePhyUe::doHandover - UE " << nodeId_ << " has completed handover to eNB " << masterId_ << "... " << endl;
    binder_->removeUeHandoverTriggered(nodeId_);

    // inform the UE's IP2Nic module to forward held packets
    IP2Nic* ip2nic =  check_and_cast<IP2Nic*>(getParentModule()->getSubmodule("ip2nic"));
    ip2nic->signalHandoverCompleteUe();

    // inform the eNB's IP2Nic module to forward data to the target eNB
    if (oldMaster != 0 && candidateMasterId_ != 0)
    {
        IP2Nic* enbIp2Nic =  check_and_cast<IP2Nic*>(getSimulation()->getModule(binder_->getOmnetId(masterId_))->getSubmodule("cellularNic")->getSubmodule("ip2nic"));
        enbIp2Nic->signalHandoverCompleteTarget(nodeId_,oldMaster);
    }
}


// TODO: ***reorganize*** method
void LtePhyUe::handleAirFrame(cMessage* msg)
{
    UserControlInfo* lteInfo = dynamic_cast<UserControlInfo*>(msg->removeControlInfo());

    if (useBattery_)
    {
        //TODO BatteryAccess::drawCurrent(rxAmount_, 0);
    }
    connectedNodeId_ = masterId_;
    LteAirFrame* frame = check_and_cast<LteAirFrame*>(msg);
    EV << "LtePhy: received new LteAirFrame with ID " << frame->getId() << " from channel" << endl;

    int sourceId = binder_->getOmnetId(lteInfo->getSourceId());
    if(sourceId == 0 )
    {
        // source has left the simulation
        delete msg;
        return;
    }

    // check if the air frame was sent on a correct carrier frequency
    double carrierFreq = lteInfo->getCarrierFrequency();
    LteChannelModel* channelModel = getChannelModel(carrierFreq);
    if (channelModel == NULL)
    {
        EV << "Received packet on carrier frequency not supported by this node. Delete it." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    //Update coordinates of this user
    if (lteInfo->getFrameType() == HANDOVERPKT)
    {
        // check if the message is on another carrier frequency or handover is already in process
        if (carrierFreq != primaryChannelModel_->getCarrierFrequency() || (handoverTrigger_ != nullptr && handoverTrigger_->isScheduled()))
        {
            EV << "Received handover packet on a different carrier frequency than the primary cell. Delete it." << endl;
            delete lteInfo;
            delete frame;
            return;
        }

        handoverHandler(frame, lteInfo);
        return;
    }

    // Check if the frame is for us ( MacNodeId matches )
    if (lteInfo->getDestId() != nodeId_)
    {
        EV << "ERROR: Frame is not for us. Delete it." << endl;
        EV << "Packet Type: " << phyFrameTypeToA((LtePhyFrameType)lteInfo->getFrameType()) << endl;
        EV << "Frame MacNodeId: " << lteInfo->getDestId() << endl;
        EV << "Local MacNodeId: " << nodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }

        /*
         * This could happen if the ue associates with a new master while the old one
         * already scheduled a packet for him: the packet is in the air while the ue changes master.
         * Event timing:      TTI x: packet scheduled and sent for UE (tx time = 1ms)
         *                     TTI x+0.1: ue changes master
         *                     TTI x+1: packet from old master arrives at ue
         */
    if (lteInfo->getSourceId() != masterId_)
    {
        EV << "WARNING: frame from an old master during handover: deleted " << endl;
        EV << "Source MacNodeId: " << lteInfo->getSourceId() << endl;
        EV << "Master MacNodeId: " << masterId_ << endl;
        delete frame;
        return;
    }

        // send H-ARQ feedback up
    if (lteInfo->getFrameType() == HARQPKT || lteInfo->getFrameType() == GRANTPKT || lteInfo->getFrameType() == RACPKT)
    {
        handleControlMsg(frame, lteInfo);
        return;
    }
    if ((lteInfo->getUserTxParams()) != nullptr)
    {
        int cw = lteInfo->getCw();
        if (lteInfo->getUserTxParams()->readCqiVector().size() == 1)
            cw = 0;
        double cqi = lteInfo->getUserTxParams()->readCqiVector()[cw];
        emit(averageCqiDl_, cqi);
    }
    // apply decider to received packet
    bool result = true;
    RemoteSet r = lteInfo->getUserTxParams()->readAntennaSet();
    if (r.size() > 1)
    {
        // DAS
        for (RemoteSet::iterator it = r.begin(); it != r.end(); it++)
        {
            EV << "LtePhy: Receiving Packet from antenna " << (*it) << "\n";

            /*
             * On UE set the sender position
             * and tx power to the sender das antenna
             */

//            cc->updateHostPosition(myHostRef,das_->getAntennaCoord(*it));
            // Set position of sender
//            Move m;
//            m.setStart(das_->getAntennaCoord(*it));
            RemoteUnitPhyData data;
            data.txPower=lteInfo->getTxPower();
            data.m=getRadioPosition();
            frame->addRemoteUnitPhyDataVector(data);
        }
        // apply analog models For DAS
        result=channelModel->isErrorDas(frame,lteInfo);
    }
    else
    {
        result = channelModel->isError(frame,lteInfo);
    }

    // update statistics
    if (result)
        numAirFrameReceived_++;
    else
        numAirFrameNotReceived_++;

    EV << "Handled LteAirframe with ID " << frame->getId() << " with result "
       << ( result ? "RECEIVED" : "NOT RECEIVED" ) << endl;

    auto pkt = check_and_cast<inet::Packet *>(frame->decapsulate());

    // here frame has to be destroyed since it is no more useful
    delete frame;

    // attach the decider result to the packet as control info
    lteInfo->setDeciderResult(result);
    *(pkt->addTagIfAbsent<UserControlInfo>()) = *lteInfo;
    delete lteInfo;

    // send decapsulated message along with result control info to upperGateOut_
    send(pkt, upperGateOut_);

    if (getEnvir()->isGUI())
    updateDisplayString();
}

void LtePhyUe::handleUpperMessage(cMessage* msg)
{
//    if (useBattery_) {
//    TODO     BatteryAccess::drawCurrent(txAmount_, 1);
//    }

    auto pkt = check_and_cast<inet::Packet *>(msg);
    auto lteInfo = pkt->getTag<UserControlInfo>();

    MacNodeId dest = lteInfo->getDestId();
    if (dest != masterId_)
    {
        // UE is not sending to its master!!
        throw cRuntimeError("LtePhyUe::handleUpperMessage  Ue preparing to send message to %d instead of its master (%d)", dest, masterId_);
    }

    double carrierFreq = lteInfo->getCarrierFrequency();
    LteChannelModel* channelModel = getChannelModel(carrierFreq);
    if (channelModel == NULL)
        throw cRuntimeError("LtePhyUe::handleUpperMessage - Carrier frequency [%f] not supported by any channel model", carrierFreq);


    if (lteInfo->getFrameType() == DATAPKT && (channelModel->isUplinkInterferenceEnabled() || channelModel->isD2DInterferenceEnabled()))
    {
        // Store the RBs used for data transmission to the binder (for UL interference computation)
        RbMap rbMap = lteInfo->getGrantedBlocks();
        Remote antenna = MACRO;  // TODO fix for multi-antenna
        binder_->storeUlTransmissionMap(channelModel->getCarrierFrequency(), antenna, rbMap, nodeId_, mac_->getMacCellId(), this, UL);
    }

    if (lteInfo->getFrameType() == DATAPKT && lteInfo->getUserTxParams() != nullptr)
    {
        double cqi = lteInfo->getUserTxParams()->readCqiVector()[lteInfo->getCw()];
        if (lteInfo->getDirection() == UL)
            emit(averageCqiUl_, cqi);
        else if (lteInfo->getDirection() == D2D)
            emit(averageCqiD2D_, cqi);
    }

    LtePhyBase::handleUpperMessage(msg);
}

double LtePhyUe::updateHysteresisTh(double v)
{
    if (hysteresisFactor_ == 0)
        return 0;
    else
        return v / hysteresisFactor_;
}

void LtePhyUe::deleteOldBuffers(MacNodeId masterId)
{
    /* Delete Mac Buffers */

    // delete macBuffer[nodeId_] at old master
    LteMacEnb *masterMac = check_and_cast<LteMacEnb *>(getMacByMacNodeId(masterId));
    masterMac->deleteQueues(nodeId_);

    // delete queues for master at this ue
    mac_->deleteQueues(masterId_);

    /* Delete Rlc UM Buffers */

    // delete UmTxQueue[nodeId_] at old master
    LteRlcUm *masterRlcUm = check_and_cast<LteRlcUm*>(getRlcByMacNodeId(masterId, UM));
    masterRlcUm->deleteQueues(nodeId_);

    // delete queues for master at this ue
    rlcUm_->deleteQueues(nodeId_);

    /* Delete PDCP Entities */
    // delete pdcpEntities[nodeId_] at old master
    LtePdcpRrcEnb* masterPdcp = check_and_cast<LtePdcpRrcEnb *>(getPdcpByMacNodeId(masterId));
    masterPdcp->deleteEntities(nodeId_);

    // delete queues for master at this ue
    pdcp_->deleteEntities(masterId_);
}

DasFilter* LtePhyUe::getDasFilter()
{
    return das_;
}

void LtePhyUe::sendFeedback(LteFeedbackDoubleVector fbDl, LteFeedbackDoubleVector fbUl, FeedbackRequest req)
{
    Enter_Method("SendFeedback");
    EV << "LtePhyUe: feedback from Feedback Generator" << endl;

    //Create a feedback packet
    auto fbPkt = makeShared<LteFeedbackPkt>();
    //Set the feedback
    fbPkt->setLteFeedbackDoubleVectorDl(fbDl);
    fbPkt->setLteFeedbackDoubleVectorDl(fbUl);
    fbPkt->setSourceNodeId(nodeId_);

    auto pkt = new Packet("feedback_pkt");
    pkt->insertAtFront(fbPkt);

    UserControlInfo* uinfo = new UserControlInfo();
    uinfo->setSourceId(nodeId_);
    uinfo->setDestId(masterId_);
    uinfo->setFrameType(FEEDBACKPKT);
    uinfo->setIsCorruptible(false);
    // create LteAirFrame and encapsulate a feedback packet
    LteAirFrame* frame = new LteAirFrame("feedback_pkt");
    frame->encapsulate(check_and_cast<cPacket*>(pkt));
    uinfo->feedbackReq = req;
    uinfo->setDirection(UL);
    simtime_t signalLength = TTI;
    uinfo->setTxPower(txPower_);
    // initialize frame fields

    frame->setSchedulingPriority(airFramePriority_);
    frame->setDuration(signalLength);

    uinfo->setCoord(getRadioPosition());

    //TODO access speed data Update channel index
//    if (coherenceTime(move.getSpeed())<(NOW-lastFeedback_)){
//        cellInfo_->channelIncrease(nodeId_);
//        cellInfo_->lambdaIncrease(nodeId_,1);
//    }
    lastFeedback_ = NOW;

    // send one feedback packet for each carrier
    std::map<double, LteChannelModel*>::iterator cit = channelModel_.begin();
    for (; cit != channelModel_.end(); ++cit)
    {
        double carrierFrequency = cit->first;
        LteAirFrame* carrierFrame = frame->dup();
        UserControlInfo* carrierInfo = uinfo->dup();
        carrierInfo->setCarrierFrequency(carrierFrequency);
        carrierFrame->setControlInfo(carrierInfo);

        EV << "LtePhy: " << nodeTypeToA(nodeType_) << " with id "
           << nodeId_ << " sending feedback to the air channel for carrier " << carrierFrequency << endl;
        sendUnicast(carrierFrame);
    }

    delete frame;
    delete uinfo;
}

void LtePhyUe::finish()
{
    if (getSimulation()->getSimulationStage() != CTX_FINISH)
    {
        // do this only at deletion of the module during the simulation

        // do this only if this PHY layer is connected to a serving base station
        if (masterId_ > 0)
        {
            // clear buffers
            deleteOldBuffers(masterId_);

            // amc calls
            LteAmc *amc = getAmcModule(masterId_);
            if (amc != nullptr)
            {
                amc->detachUser(nodeId_, UL);
                amc->detachUser(nodeId_, DL);
            }

            // binder call
            binder_->unregisterNextHop(masterId_, nodeId_);

            // cellInfo call
            cellInfo_->detachUser(nodeId_);
        }
    }
}
