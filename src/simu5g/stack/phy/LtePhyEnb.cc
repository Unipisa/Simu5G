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

#include <inet/networklayer/common/NetworkInterface.h>

#include "simu5g/stack/phy/LtePhyEnb.h"
#include "simu5g/stack/phy/feedback/LteUlFeedbackGenerator.h"
#include "simu5g/stack/phy/packet/LteFeedbackPkt.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

Define_Module(LtePhyEnb);

using namespace omnetpp;
using namespace inet;


LtePhyEnb::~LtePhyEnb()
{
    cancelAndDelete(bdcStarter_);
    cancelAndDelete(csiRsStarter_);
    delete lteFeedbackComputation_;
}

void LtePhyEnb::initialize(int stage)
{
    LtePhyBase::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        // get local id
        nodeId_ = MacNodeId(hostModule->par("macNodeId").intValue());
        EV << "Local MacNodeId: " << nodeId_ << endl;
        useTransparentNtn_ = hostModule->hasPar("useTransparentNtn") && hostModule->par("useTransparentNtn").boolValue();
        ntnInGate_ = gate("ntnIn");
        ntnOutGate_ = gate("ntnOut");
        isNr_ = (std::string(getContainingNicModule(this)->getComponentType()->getName()) == "NrNicEnb");

        randomChannelIndex_ = intuniform(1, binder_->phyPisaData.maxChannel2()); // NOTE: moving this to the next stage (where it is used will change random number stream and CHANGE FINGERPRINTS!

        nodeType_ = NODEB;
        WATCH(nodeType_);
    }
    else if (stage == INITSTAGE_SIMU5G_BINDER_ACCESS) {
        cellInfo_ = binder_->getCellInfoByNodeId(nodeId_);
    }
    else if (stage == INITSTAGE_SIMU5G_PHYSICAL_LAYER) {
        initializeFeedbackComputation();
        ulFbGen_.reference(this, "ulFeedbackGeneratorModule", true);

        csiRsPeriod_ = simtime_t(int(par("csiRsPeriod")) * TTI);
        srsPeriod_ = simtime_t(int(par("srsPeriod")) * TTI);

        //check eNb type and set TX power
        if (cellInfo_->getEnbType() == MICRO_ENB)
            txPower_ = microTxPower_;
        else
            txPower_ = eNodeBtxPower_;

        // set TX direction
        std::string txDir = par("txDirection");
        txDirection_ = static_cast<TxDirectionType>(cEnum::get("simu5g::TxDirectionType")->lookup(txDir.c_str()));
        switch (txDirection_) {
            case OMNI: txAngle_ = 0.0;
                break;
            case ANISOTROPIC: txAngle_ = par("txAngle");
                break;
            default: throw cRuntimeError("unknown txDirection: '%s'", txDir.c_str());
        }

        bdcUpdateInterval_ = cellInfo_->par("broadcastMessageInterval");
        if (bdcUpdateInterval_ != 0 && par("enableHandover").boolValue()) {
            // self message provoking the generation of a broadcast message
            bdcStarter_ = new cMessage("bdcStarter");
            scheduleAt(NOW, bdcStarter_);
        }
        if (csiRsPeriod_ != 0) {
            csiRsStarter_ = new cMessage("csiRsStarter");
            scheduleAt(NOW, csiRsStarter_);
        }
    }
}

void LtePhyEnb::handleSelfMessage(cMessage *msg)
{
    if (msg->isName("bdcStarter")) {
        // send broadcast message
        LteAirFrame *f = createHandoverMessage();
        sendBroadcast(f);
        scheduleAt(NOW + bdcUpdateInterval_, msg);
    }
    else if (msg->isName("csiRsStarter")) {
        for (const auto& [carrierFrequency, channelModel] : channelModel_) {
            LteAirFrame *f = createCsiReferenceSignalFrame(carrierFrequency);
            sendCsiReferenceSignalFrameToAttachedUes(f);
        }
        scheduleAt(NOW + csiRsPeriod_, msg);
    }
    else {
        delete msg;
    }
}

void LtePhyEnb::sendNtn(LteAirFrame *airFrame)
{
    if (ntnOutGate_ == nullptr || !ntnOutGate_->isConnected())
        throw cRuntimeError("LtePhyEnb::sendNtn - NTN fronthaul gate is not connected for %s", getFullPath().c_str());

    if (airFrame->getControlInfo() != nullptr) {
        UserControlInfo *userControlInfo = check_and_cast<UserControlInfo *>(airFrame->removeControlInfo());
        airFrame->setAdditionalInfo(*userControlInfo);
        delete userControlInfo;
    }

    send(airFrame, ntnOutGate_);
}

void LtePhyEnb::sendBroadcast(LteAirFrame *airFrame)
{
    if (useTransparentNtn_)
        sendNtn(airFrame);
    else
        LtePhyBase::sendBroadcast(airFrame);
}

void LtePhyEnb::sendUnicast(LteAirFrame *airFrame)
{
    if (useTransparentNtn_)
        sendNtn(airFrame);
    else
        LtePhyBase::sendUnicast(airFrame);
}

bool LtePhyEnb::handleControlPkt(UserControlInfo *lteinfo, LteAirFrame *frame)
{
    EV << "Received control packet " << endl;
    MacNodeId senderMacNodeId = lteinfo->getSourceId();
    if (!binder_->nodeExists(senderMacNodeId)) {
        EV << "Sender (" << senderMacNodeId << ") does not exist anymore!" << std::endl;
        delete frame;
        return true;    // FIXME ? make sure that nodes that left the simulation do not send
    }
    if (lteinfo->getFrameType() == HANDOVERPKT) {
        // handover broadcast frames must not be relayed or processed by eNB
        delete frame;
        return true;
    }
    // send H-ARQ feedback up
    if (lteinfo->getFrameType() == HARQPKT
        || lteinfo->getFrameType() == RACPKT)
    {
        handleControlMsg(frame, lteinfo);
        return true;
    }
    //handle feedback packet
    if (lteinfo->getFrameType() == FEEDBACKPKT) {
        handleFeedbackPkt(lteinfo, frame);
        delete frame;
        return true;
    }
    if (lteinfo->getFrameType() == SRSPKT) {
        handleSrsReferenceSignal(lteinfo, frame);
        return true;
    }
    return false;
}

void LtePhyEnb::handleAirFrame(cMessage *msg)
{
    if (msg->getArrivalGate() == ntnInGate_) {
        auto *pkt = check_and_cast<Packet *>(msg);
        auto *frame = check_and_cast<LteAirFrame *>(pkt->decapsulate());
        delete pkt;
        handleAirFrame(frame);
        return;
    }

    LteAirFrame *frame = static_cast<LteAirFrame *>(msg);
    UserControlInfo *lteInfo = new UserControlInfo(frame->getAdditionalInfo());

    EV << "LtePhy: received new LteAirFrame with ID " << frame->getId() << " from channel" << endl;

    // handle broadcast packet sent by another eNB
    if (lteInfo->getFrameType() == HANDOVERPKT) {
        EV << "LtePhyEnb::handleAirFrame - received handover packet from another eNodeB. Ignore it." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    // check if the air frame was sent on a correct carrier frequency
    GHz carrierFreq = lteInfo->getCarrierFrequency();
    LteChannelModel *channelModel = getChannelModel(carrierFreq);
    if (channelModel == nullptr) {
        EV << "Received packet on carrier frequency not supported by this node. Delete it." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    /*
     * This could happen if the ue associates with a new master while it has
     * already scheduled a packet for the old master: the packet is in the air
     * while the ue changes master.
     * Event timing:      TTI x: packet scheduled and sent by the UE (tx time = 1ms)
     *                     TTI x+0.1: ue changes master
     *                     TTI x+1: packet from UE arrives at the old master
     */
    if (binder_->getNextHop(lteInfo->getSourceId()) != nodeId_) {
        EV << "WARNING: frame from a UE that is leaving this cell (handover): deleted " << endl;
        EV << "Source MacNodeId: " << lteInfo->getSourceId() << endl;
        EV << "Master MacNodeId: " << nodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    connectedNodeId_ = lteInfo->getSourceId();

    if (!binder_->nodeExists(connectedNodeId_) || !binder_->nodeExists(lteInfo->getDestId())) {
        // either source or destination have left the simulation
        delete msg;
        return;
    }

    //handle all control packets
    if (handleControlPkt(lteInfo, frame))
        return; // If frame contains a control packet no further action is needed

    // DAS removed - single antenna only
    bool result = channelModel->isReceptionSuccessful(frame, lteInfo);
    if (result)
        numAirFrameReceived_++;
    else
        numAirFrameNotReceived_++;

    EV << "Handled LteAirframe with ID " << frame->getId() << " with result "
       << (result ? "RECEIVED" : "NOT RECEIVED") << endl;

    auto pkt = check_and_cast<inet::Packet *>(frame->decapsulate());

    // here frame has to be destroyed since it is no more useful
    delete frame;

    // attach the decider result to the packet as control info
    *(pkt->addTagIfAbsent<UserControlInfo>()) = *lteInfo;
    delete lteInfo;

    pkt->addTagIfAbsent<PhyReceptionInd>()->setDeciderResult(result);

    // send decapsulated message along with result control info to upperGateOut_
    send(pkt, upperGateOut_);

    if (getEnvir()->isGUI())
        updateDisplayString();
}

LteAirFrame *LtePhyEnb::createCsiReferenceSignalFrame(GHz carrierFrequency)
{
    LteAirFrame *csiAirFrame = new LteAirFrame("CsiReferenceSignal");
    UserControlInfo *cInfo = new UserControlInfo();
    cInfo->setSourceId(nodeId_);
    cInfo->setFrameType(CSIRSPKT);
    cInfo->setDirection(DL);
    cInfo->setTxPower(txPower_);
    cInfo->setCarrierFrequency(carrierFrequency);
    cInfo->setIsNr(isNr_);
    cInfo->setCoord(getRadioPosition());
    csiAirFrame->setControlInfo(cInfo);
    csiAirFrame->setDuration(TTI);
    csiAirFrame->setSchedulingPriority(airFramePriority_);
    return csiAirFrame;
}

void LtePhyEnb::sendCsiReferenceSignalFrameToAttachedUes(LteAirFrame *frame)
{
    UserControlInfo *ci = check_and_cast<UserControlInfo *>(frame->getControlInfo());
    frame->setAdditionalInfo(*ci);
    delete frame->removeControlInfo();

    for (MacNodeId ueId : cellInfo_->getAttachedUes()) {
        if (isNrUe(ueId) != isNr_)
            continue;

        cModule *receiver = binder_->getNodeModule(ueId);
        if (receiver == nullptr)
            continue;

        LteAirFrame *frameToSend = frame->dup();
        sendDirect(frameToSend, 0, frame->getDuration(), receiver, getReceiverGateIndex(receiver, isNrUe(ueId)));
    }

    delete frame;
}

void LtePhyEnb::handleSrsReferenceSignal(UserControlInfo *lteinfo, LteAirFrame *frame)
{
    EV_DEBUG << "LtePhyEnb::handleSrsReferenceSignal - received SRS frame with ID " << frame->getId() << endl;
    EV_INFO << "LtePhyEnb::handleSrsReferenceSignal - computing UL CSI for node " << lteinfo->getSourceId() << endl;

    LteFeedbackDoubleVector ulFeedback = ulFbGen_->computeUlFeedback(lteinfo, frame);

    // create feedback packet for MAC layer
    // TODO make this an indication
    auto header = makeShared<LteFeedbackPkt>();
    header->setSourceNodeId(lteinfo->getSourceId());
    header->setLteFeedbackDoubleVectorUl(ulFeedback);

    auto pkt = new Packet("feedback_pkt");
    pkt->insertAtFront(header);

    auto tag = pkt->addTagIfAbsent<UserControlInfo>();
    *tag = *lteinfo;
    tag->setFrameType(FEEDBACKPKT);

    delete lteinfo;
    delete frame;

    EV_INFO << "LtePhyEnb::handleSrsReferenceSignal - forward Feedback Packet to MAC layer" << endl;
    send(pkt, upperGateOut_);
}

void LtePhyEnb::handleFeedbackPkt(UserControlInfo *lteinfo, LteAirFrame *frame)
{
    EV << "LtePhyEnb::handleFeedbackPkt - received Feedback Packet with ID " << frame->getId() << endl;
    auto pktAux = check_and_cast<Packet *>(frame->decapsulate());
    auto header = pktAux->removeAtFront<LteFeedbackPkt>();

    *(pktAux->addTagIfAbsent<UserControlInfo>()) = *lteinfo;

    // if feedback was generated elsewhere we can send up to mac, othewise nodeb should generate the feedback here
    if (lteinfo->getFeedbackReq().request)
    {
        EV_DEBUG << "LtePhyEnb::handleFeedbackPkt - received feedback computation request from node " << lteinfo->getSourceId() << endl;

        LteFeedbackDoubleVector fb;

        EV_INFO << "LtePhyEnb::handleFeedbackPkt - computing UL CSI for node " << lteinfo->getSourceId() << endl;
        fb = ulFbGen_->computeUlFeedback(lteinfo, frame);
        header->setLteFeedbackDoubleVectorUl(fb);

        // compute DL CSI feedback at the e/gNodeB side
        EV_INFO << "LtePhyEnb::handleFeedbackPkt - computing DL CSI for node " << lteinfo->getSourceId() << endl;

        FeedbackRequest req = lteinfo->getFeedbackReq();
        LteChannelModel *channelModel = getChannelModel(lteinfo->getCarrierFrequency());
        if (channelModel == nullptr)
            throw cRuntimeError("LtePhyEnb::handleFeedbackPkt - channelModel is a null pointer");

        int nRus = 0;
        std::map<Remote, int> antennaCws;
        antennaCws[MACRO] = 1;
        unsigned int numPreferredBand = cellInfo_->getNumPreferredBands();

        lteinfo->setTxPower(txPower_);
        lteinfo->setDirection(DL);

        std::vector<double> snr = channelModel->getSINR(frame, lteinfo);
        fb = lteFeedbackComputation_->computeFeedback(req.type, req.rbAllocationType, req.txMode,
                antennaCws, numPreferredBand, nRus, snr, lteinfo->getSourceId());
        header->setLteFeedbackDoubleVectorDl(fb);

        EV << "LtePhyEnb::handleFeedbackPkt : Pisa Feedback Generated at nodeId: " << nodeId_ << " Feedback size: " << fb.size()
           << " Carrier: " << lteinfo->getCarrierFrequency() << endl;

        // DEBUG
        bool debug = false;
        if (debug) {
            LteFeedbackDoubleVector vec = header->getLteFeedbackDoubleVectorDl();
            for (const auto& feedbackDouble : vec) {
                for (const auto& feedback : feedbackDouble) {
                    MacNodeId id = lteinfo->getSourceId();
                    EV_DEBUG << endl << "Node:" << id << endl;
                    TxMode t = feedback.getTxMode();
                    EV_DEBUG << "TXMODE: " << txModeToA(t) << endl;
                    if (feedback.hasBandCqi()) {
                        const std::vector<CqiVector>& bandCqiVec = feedback.getBandCqi();
                        for (const auto& bandCqi : bandCqiVec) {
                            int i;
                            for (i = 0; i < bandCqi.size(); ++i)
                                EV_DEBUG << "Band " << i << " CQI " << bandCqi[i] << endl;
                        }
                    }
                    else if (feedback.hasWbCqi()) {
                        const CqiVector& widebandCqi = feedback.getWbCqi();
                        for (const auto& cqi : widebandCqi)
                            EV_DEBUG << "wideband CQI " << cqi << endl;
                    }
                    if (feedback.hasRankIndicator()) {
                        EV_DEBUG << "Rank " << feedback.getRankIndicator() << endl;
                    }
                }
            }
        }
    }
    pktAux->insertAtFront(header);
    delete lteinfo;

    // send decapsulated message along with result control info to upperGateOut_
    EV_INFO << "LtePhyEnb::handleFeedbackPkt - forward Feedback Packet to MAC layer" << endl;
    send(pktAux, upperGateOut_);
}

void LtePhyEnb::initializeFeedbackComputation()
{
    const char *name = "REAL";

    double targetBler = par("targetBler");

    // compute feedback for the primary carrier only
    // TODO add support for feedback computation for all carriers
    lteFeedbackComputation_ = new LteFeedbackComputationRealistic(
            binder_,
            targetBler, cellInfo_->getPrimaryCarrierNumBands());

    EV << "Feedback Computation \"" << name << "\" loaded." << endl;
}

} //namespace
