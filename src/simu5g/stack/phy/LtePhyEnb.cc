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

#include "stack/phy/LtePhyEnb.h"
#include "stack/phy/packet/LteFeedbackPkt.h"
#include "stack/phy/das/DasFilter.h"
#include "common/LteCommon.h"

namespace simu5g {

Define_Module(LtePhyEnb);

using namespace omnetpp;
using namespace inet;


LtePhyEnb::~LtePhyEnb()
{
    cancelAndDelete(bdcStarter_);
    delete lteFeedbackComputation_;
    delete das_;
}

DasFilter *LtePhyEnb::getDasFilter()
{
    return das_;
}

void LtePhyEnb::initialize(int stage)
{
    LtePhyBase::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        // get local id
        nodeId_ = MacNodeId(hostModule->par("macNodeId").intValue());
        EV << "Local MacNodeId: " << nodeId_ << endl;

        cellInfo_ = getCellInfo(binder_, nodeId_);
        if (cellInfo_ != nullptr) {
            cellInfo_->channelUpdate(nodeId_, intuniform(1, binder_->phyPisaData.maxChannel2()));
            das_ = new DasFilter(this, binder_, cellInfo_->getRemoteAntennaSet(), 0);
        }
        isNr_ = (std::string(getContainingNicModule(this)->getComponentType()->getName()) == "NRNicEnb");

        nodeType_ = (isNr_) ? GNODEB : ENODEB;
        WATCH(nodeType_);
    }
    else if (stage == 1) {
        initializeFeedbackComputation();

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
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
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
    else {
        delete msg;
    }
}

bool LtePhyEnb::handleControlPkt(UserControlInfo *lteinfo, LteAirFrame *frame)
{
    EV << "Received control packet " << endl;
    MacNodeId senderMacNodeId = lteinfo->getSourceId();
    if (binder_->getOmnetId(senderMacNodeId) == 0) {
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
    return false;
}

void LtePhyEnb::handleAirFrame(cMessage *msg)
{
    UserControlInfo *lteInfo = check_and_cast<UserControlInfo *>(msg->removeControlInfo());
    if (lteInfo == nullptr) {
        return;
    }

    LteAirFrame *frame = static_cast<LteAirFrame *>(msg);

    EV << "LtePhy: received new LteAirFrame with ID " << frame->getId() << " from channel" << endl;

    // handle broadcast packet sent by another eNB
    if (lteInfo->getFrameType() == HANDOVERPKT) {
        EV << "LtePhyEnb::handleAirFrame - received handover packet from another eNodeB. Ignore it." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    // check if the air frame was sent on a correct carrier frequency
    double carrierFreq = lteInfo->getCarrierFrequency();
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

    int sourceId = binder_->getOmnetId(connectedNodeId_);
    int senderId = binder_->getOmnetId(lteInfo->getDestId());
    if (sourceId == 0 || senderId == 0) {
        // either source or destination have left the simulation
        delete msg;
        return;
    }

    //handle all control packets
    if (handleControlPkt(lteInfo, frame))
        return; // If frame contains a control packet no further action is needed

    bool result = true;
    RemoteSet r = lteInfo->getUserTxParams()->readAntennaSet();
    if (r.size() > 1) {
        // Use DAS
        // Message from ue
        for (auto it : r) {
            EV << "LtePhy: Receiving Packet from antenna " << it << "\n";

            /*
             * On eNodeB set the current position
             * to the receiving das antenna
             */
            cc->setRadioPosition(myRadioRef, das_->getAntennaCoord(it));

            RemoteUnitPhyData data;
            data.txPower = lteInfo->getTxPower();
            data.m = getRadioPosition();
            frame->addRemoteUnitPhyDataVector(data);
        }
        result = channelModel->isErrorDas(frame, lteInfo);
    }
    else {
        result = channelModel->isError(frame, lteInfo);
    }
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
    lteInfo->setDeciderResult(result);
    *(pkt->addTagIfAbsent<UserControlInfo>()) = *lteInfo;
    delete lteInfo;

    // send decapsulated message along with result control info to upperGateOut_
    send(pkt, upperGateOut_);

    if (getEnvir()->isGUI())
        updateDisplayString();
}

void LtePhyEnb::requestFeedback(UserControlInfo *lteinfo, LteAirFrame *frame, Packet *pktAux)
{
    EV << NOW << " LtePhyEnb::requestFeedback " << endl;
    LteFeedbackDoubleVector fb;

    // select the correct channel model according to the carrier frequency
    LteChannelModel *channelModel = getChannelModel(lteinfo->getCarrierFrequency());

    //get UE Position
    Coord sendersPos = lteinfo->getCoord();
    cellInfo_->setUePosition(lteinfo->getSourceId(), sendersPos);

    std::vector<double> snr;
    auto header = pktAux->removeAtFront<LteFeedbackPkt>();

    //Apply analog model (path loss)
    //Get snr for UL direction
    if (channelModel != nullptr)
        snr = channelModel->getSINR(frame, lteinfo);
    else
        throw cRuntimeError("LtePhyEnbD2D::requestFeedback - channelModel is a null pointer. Abort");

    FeedbackRequest req = lteinfo->feedbackReq;
    //Feedback computation
    fb.clear();
    //get number of RU
    int nRus = cellInfo_->getNumRus();
    TxMode txmode = req.txMode;
    FeedbackType type = req.type;
    RbAllocationType rbtype = req.rbAllocationType;
    std::map<Remote, int> antennaCws = cellInfo_->getAntennaCws();
    unsigned int numPreferredBand = cellInfo_->getNumPreferredBands();

    for (Direction dir = UL; dir != UNKNOWN_DIRECTION;
         dir = ((dir == UL) ? DL : UNKNOWN_DIRECTION))
    {
        //for each RU is called the computation feedback function
        if (req.genType == IDEAL) {
            fb = lteFeedbackComputation_->computeFeedback(type, rbtype, txmode,
                    antennaCws, numPreferredBand, IDEAL, nRus, snr,
                    lteinfo->getSourceId());
        }
        else if (req.genType == REAL) {
            fb.resize(das_->getReportingSet().size());
            for (const auto &remote : das_->getReportingSet())
            {
                fb[remote].resize((int)txmode);
                fb[remote][(int)txmode] =
                    lteFeedbackComputation_->computeFeedback(remote, txmode,
                            type, rbtype, antennaCws[remote], numPreferredBand,
                            REAL, nRus, snr, lteinfo->getSourceId());
            }
        }
        // the reports are computed only for the antenna in the reporting set
        else if (req.genType == DAS_AWARE) {
            fb.resize(das_->getReportingSet().size());
            for (const auto &remote : das_->getReportingSet())
            {
                fb[remote] = lteFeedbackComputation_->computeFeedback(remote, type,
                        rbtype, txmode, antennaCws[remote], numPreferredBand,
                        DAS_AWARE, nRus, snr, lteinfo->getSourceId());
            }
        }

        if (dir == UL) {
            header->setLteFeedbackDoubleVectorUl(fb);
            //Prepare  parameters for next loop iteration - in order to compute SNR in DL
            lteinfo->setTxPower(txPower_);
            lteinfo->setDirection(DL);

            //Get snr for DL direction
            if (channelModel != nullptr)
                snr = channelModel->getSINR(frame, lteinfo);
            else
                throw cRuntimeError("LtePhyEnbD2D::requestFeedback - channelModel is a null pointer. Abort");
        }
        else
            header->setLteFeedbackDoubleVectorDl(fb);
    }
    EV << "LtePhyEnb::requestFeedback : Pisa Feedback Generated for nodeId: "
       << nodeId_ << " with generator type "
       << fbGeneratorTypeToA(req.genType) << " Feedback size: " << fb.size()
       << " Carrier: " << lteinfo->getCarrierFrequency() << endl;

    pktAux->insertAtFront(header);
}

void LtePhyEnb::handleFeedbackPkt(UserControlInfo *lteinfo,
        LteAirFrame *frame)
{
    EV << "Handled Feedback Packet with ID " << frame->getId() << endl;
    auto pktAux = check_and_cast<Packet *>(frame->decapsulate());
    auto header = pktAux->peekAtFront<LteFeedbackPkt>();

    *(pktAux->addTagIfAbsent<UserControlInfo>()) = *lteinfo;

    // if feedback was generated by dummy phy we can send up to mac else nodeb should generate the "real" feedback
    if (lteinfo->feedbackReq.request) {
        requestFeedback(lteinfo, frame, pktAux);

        // DEBUG
        bool debug = false;
        if (debug) {
            LteFeedbackDoubleVector vec = header->getLteFeedbackDoubleVectorDl();
            for (const auto& feedbackDouble : vec) {
                for (const auto& feedback : feedbackDouble) {
                    MacNodeId id = lteinfo->getSourceId();
                    EV << endl << "Node:" << id << endl;
                    TxMode t = feedback.getTxMode();
                    EV << "TXMODE: " << txModeToA(t) << endl;
                    if (feedback.hasBandCqi()) {
                        const std::vector<CqiVector>& bandCqiVec = feedback.getBandCqi();
                        for (const auto& bandCqi : bandCqiVec) {
                            int i;
                            for (i = 0; i < bandCqi.size(); ++i)
                                EV << "Band " << i << " CQI " << bandCqi[i] << endl;
                        }
                    }
                    else if (feedback.hasWbCqi()) {
                        const CqiVector& widebandCqi = feedback.getWbCqi();
                        for (const auto& cqi : widebandCqi)
                            EV << "wideband CQI " << cqi << endl;
                    }
                    if (feedback.hasRankIndicator()) {
                        EV << "Rank " << feedback.getRankIndicator() << endl;
                    }
                }
            }
        }
    }
    delete lteinfo;
    // send decapsulated message along with result control info to upperGateOut_
    send(pktAux, upperGateOut_);
}

// TODO adjust default value
LteFeedbackComputation *LtePhyEnb::getFeedbackComputationFromName(std::string name, ParameterMap& params)
{
    ParameterMap::iterator it;
    if (name == "REAL") {
        // default value
        double targetBler = 0.1;
        double lambdaMinTh = 0.02;
        double lambdaMaxTh = 0.2;
        double lambdaRatioTh = 20;
        it = params.find("targetBler");
        if (it != params.end()) {
            targetBler = params["targetBler"].doubleValue();
        }
        it = params.find("lambdaMinTh");
        if (it != params.end()) {
            lambdaMinTh = params["lambdaMinTh"].doubleValue();
        }
        it = params.find("lambdaMaxTh");
        if (it != params.end()) {
            lambdaMaxTh = params["lambdaMaxTh"].doubleValue();
        }
        it = params.find("lambdaRatioTh");
        if (it != params.end()) {
            lambdaRatioTh = params["lambdaRatioTh"].doubleValue();
        }
        LteFeedbackComputation *fbcomp = new LteFeedbackComputationRealistic(
                binder_,
                targetBler, cellInfo_->getLambda(), lambdaMinTh, lambdaMaxTh,
                lambdaRatioTh, cellInfo_->getNumBands());
        return fbcomp;
    }
    else
        return nullptr;
}

void LtePhyEnb::initializeFeedbackComputation()
{
    const char *name = "REAL";

    double targetBler = par("targetBler");
    double lambdaMinTh = par("lambdaMinTh");
    double lambdaMaxTh = par("lambdaMaxTh");
    double lambdaRatioTh = par("lambdaRatioTh");

    // compute feedback for the primary carrier only
    // TODO add support for feedback computation for all carriers
    lteFeedbackComputation_ = new LteFeedbackComputationRealistic(
            binder_,
            targetBler, cellInfo_->getLambda(), lambdaMinTh, lambdaMaxTh,
            lambdaRatioTh, cellInfo_->getPrimaryCarrierNumBands());

    EV << "Feedback Computation \"" << name << "\" loaded." << endl;
}

} //namespace

