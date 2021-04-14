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

#include "stack/phy/layer/LtePhyEnb.h"
#include "stack/phy/packet/LteFeedbackPkt.h"
#include "stack/phy/das/DasFilter.h"
#include "common/LteCommon.h"

Define_Module(LtePhyEnb);

using namespace omnetpp;
using namespace inet;

LtePhyEnb::LtePhyEnb()
{
    das_ = nullptr;
    bdcStarter_ = nullptr;
}

LtePhyEnb::~LtePhyEnb()
{
    cancelAndDelete(bdcStarter_);
    if(lteFeedbackComputation_){
        delete lteFeedbackComputation_;
        lteFeedbackComputation_ = nullptr;
    }
    if(das_){
        delete das_;
        das_ = nullptr;
    }
}

DasFilter* LtePhyEnb::getDasFilter()
{
    return das_;
}

void LtePhyEnb::initialize(int stage)
{
    LtePhyBase::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL)
    {
        // get local id
        nodeId_ = getAncestorPar("macNodeId");
        EV << "Local MacNodeId: " << nodeId_ << endl;
        cellInfo_ = getCellInfo(nodeId_);
        if (cellInfo_ != NULL)
        {
            cellInfo_->channelUpdate(nodeId_, intuniform(1, binder_->phyPisaData.maxChannel2()));
            das_ = new DasFilter(this, binder_, cellInfo_->getRemoteAntennaSet(), 0);
        }
        isNr_ = (strcmp(getAncestorPar("nicType").stdstringValue().c_str(),"NRNicEnb") == 0) ? true : false;

        nodeType_ = (isNr_) ? GNODEB : ENODEB;
    }
    else if (stage == 1)
    {
        initializeFeedbackComputation();

        //check eNb type and set TX power
        if (cellInfo_->getEnbType() == MICRO_ENB)
            txPower_ = microTxPower_;
        else
            txPower_ = eNodeBtxPower_;

        // set TX direction
        std::string txDir = par("txDirection");
        if (txDir.compare(txDirections[OMNI].txDirectionName)==0)
        {
            txDirection_ = OMNI;
        }
        else   // ANISOTROPIC
        {
            txDirection_ = ANISOTROPIC;

            // set TX angle
            txAngle_ = par("txAngle");
        }

        bdcUpdateInterval_ = cellInfo_->par("broadcastMessageInterval");
        if (bdcUpdateInterval_ != 0 && par("enableHandover").boolValue()) {
            // self message provoking the generation of a broadcast message
            bdcStarter_ = new cMessage("bdcStarter");
            scheduleAt(NOW, bdcStarter_);
        }
    }
    else if (stage == INITSTAGE_LINK_LAYER)
    {

   }
}

void LtePhyEnb::handleSelfMessage(cMessage *msg)
{
    if (msg->isName("bdcStarter"))
    {
        // send broadcast message
        LteAirFrame *f = createHandoverMessage();
        sendBroadcast(f);
        scheduleAt(NOW + bdcUpdateInterval_, msg);
    }
    else
    {
        delete msg;
    }
}

bool LtePhyEnb::handleControlPkt(UserControlInfo* lteinfo, LteAirFrame* frame)
{
    EV << "Received control pkt " << endl;
    MacNodeId senderMacNodeId = lteinfo->getSourceId();
    if (binder_->getOmnetId(senderMacNodeId) == 0)
    {
        EV << "Sender (" << senderMacNodeId << ") does not exist anymore!" << std::endl;
        delete frame;
        return true;    // FIXME ? make sure that nodes that left the simulation do not send
    }
    if (lteinfo->getFrameType() == HANDOVERPKT)
    {
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
    //handle feedback pkt
    if (lteinfo->getFrameType() == FEEDBACKPKT)
    {
        handleFeedbackPkt(lteinfo, frame);
        delete frame;
        return true;
    }
    return false;
}

void LtePhyEnb::handleAirFrame(cMessage* msg)
{
    UserControlInfo* lteInfo = check_and_cast<UserControlInfo*>(msg->removeControlInfo());
    if (!lteInfo)
    {
        return;
    }

    LteAirFrame* frame = static_cast<LteAirFrame*>(msg);

    EV << "LtePhy: received new LteAirFrame with ID " << frame->getId() << " from channel" << endl;

    // handle broadcast packet sent by another eNB
    if (lteInfo->getFrameType() == HANDOVERPKT)
    {
        EV << "LtePhyEnb::handleAirFrame - received handover packet from another eNodeB. Ignore it." << endl;
        delete lteInfo;
        delete frame;
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

    /*
     * This could happen if the ue associates with a new master while it has
     * already scheduled a packet for the old master: the packet is in the air
     * while the ue changes master.
     * Event timing:      TTI x: packet scheduled and sent by the UE (tx time = 1ms)
     *                     TTI x+0.1: ue changes master
     *                     TTI x+1: packet from UE arrives at the old master
     */
    if (binder_->getNextHop(lteInfo->getSourceId()) != nodeId_)
    {
        EV << "WARNING: frame from a UE that is leaving this cell (handover): deleted " << endl;
        EV << "Source MacNodeId: " << lteInfo->getSourceId() << endl;
        EV << "Master MacNodeId: " << nodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    connectedNodeId_ = lteInfo->getSourceId();

    int sourceId = getBinder()->getOmnetId(connectedNodeId_);
    int senderId = getBinder()->getOmnetId(lteInfo->getDestId());
    if(sourceId == 0 || senderId == 0)
    {
        // either source or destination have left the simulation
        delete msg;
        return;
    }

    //handle all control pkt
    if (handleControlPkt(lteInfo, frame))
        return; // If frame contain a control pkt no further action is needed

    bool result = true;
    RemoteSet r = lteInfo->getUserTxParams()->readAntennaSet();
    if (r.size() > 1)
    {
        // Use DAS
        // Message from ue
        for (RemoteSet::iterator it = r.begin(); it != r.end(); it++)
        {
            EV << "LtePhy: Receiving Packet from antenna " << (*it) << "\n";

            /*
             * On eNodeB set the current position
             * to the receiving das antenna
             */
            cc->setRadioPosition(myRadioRef, das_->getAntennaCoord(*it));

            RemoteUnitPhyData data;
            data.txPower = lteInfo->getTxPower();
            data.m = getRadioPosition();
            frame->addRemoteUnitPhyDataVector(data);
        }
        result = channelModel->isErrorDas(frame, lteInfo);
    }
    else
    {
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
void LtePhyEnb::requestFeedback(UserControlInfo* lteinfo, LteAirFrame* frame, Packet* pktAux)
{
    EV << NOW << " LtePhyEnb::requestFeedback " << endl;
    LteFeedbackDoubleVector fb;

    // select the correct channel model according to the carrier freq
    LteChannelModel* channelModel = getChannelModel(lteinfo->getCarrierFrequency());

    //get UE Position
    Coord sendersPos = lteinfo->getCoord();
    cellInfo_->setUePosition(lteinfo->getSourceId(), sendersPos);

    std::vector<double> snr;
    auto header = pktAux->removeAtFront<LteFeedbackPkt>();

    //Apply analog model (pathloss)
    //Get snr for UL direction
    if (channelModel != NULL)
        snr = channelModel->getSINR(frame, lteinfo);
    else
        throw cRuntimeError("LtePhyEnbD2D::requestFeedback - channelModel is null pointer. Abort");

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
        dir = ((dir == UL )? DL : UNKNOWN_DIRECTION))
    {
        //for each RU is called the computation feedback function
        if (req.genType == IDEAL)
        {
            fb = lteFeedbackComputation_->computeFeedback(type, rbtype, txmode,
                antennaCws, numPreferredBand, IDEAL, nRus, snr,
                lteinfo->getSourceId());
        }
        else if (req.genType == REAL)
        {
            RemoteSet::iterator it;
            fb.resize(das_->getReportingSet().size());
            for (it = das_->getReportingSet().begin();
                it != das_->getReportingSet().end(); ++it)
            {
                fb[(*it)].resize((int) txmode);
                fb[(*it)][(int) txmode] =
                lteFeedbackComputation_->computeFeedback(*it, txmode,
                    type, rbtype, antennaCws[*it], numPreferredBand,
                    REAL, nRus, snr, lteinfo->getSourceId());
            }
        }
        // the reports are computed only for the antenna in the reporting set
        else if (req.genType == DAS_AWARE)
        {
            RemoteSet::iterator it;
            fb.resize(das_->getReportingSet().size());
            for (it = das_->getReportingSet().begin();
                it != das_->getReportingSet().end(); ++it)
            {
                fb[(*it)] = lteFeedbackComputation_->computeFeedback(*it, type,
                    rbtype, txmode, antennaCws[*it], numPreferredBand,
                    DAS_AWARE, nRus, snr, lteinfo->getSourceId());
            }
        }


        if (dir == UL)
        {
            header->setLteFeedbackDoubleVectorUl(fb);
            //Prepare  parameters for next loop iteration - in order to compute SNR in DL
            lteinfo->setTxPower(txPower_);
            lteinfo->setDirection(DL);

            //Get snr for DL direction
            if (channelModel != NULL)
                snr = channelModel->getSINR(frame, lteinfo);
            else
                throw cRuntimeError("LtePhyEnbD2D::requestFeedback - channelModel is null pointer. Abort");
        }
        else
            header->setLteFeedbackDoubleVectorDl(fb);
    }
    EV << "LtePhyEnb::requestFeedback : Pisa Feedback Generated for nodeId: "
       << nodeId_ << " with generator type "
       << fbGeneratorTypeToA(req.genType) << " Fb size: " << fb.size()
       << " Carrier: " << lteinfo->getCarrierFrequency() << endl;

    pktAux->insertAtFront(header);
}

void LtePhyEnb::handleFeedbackPkt(UserControlInfo* lteinfo,
    LteAirFrame *frame)
{
    EV << "Handled Feedback Packet with ID " << frame->getId() << endl;
    auto pktAux = check_and_cast<Packet *>(frame->decapsulate());
    auto header =  pktAux->peekAtFront<LteFeedbackPkt>();

    *(pktAux->addTagIfAbsent<UserControlInfo>()) = *lteinfo;

    // if feedback was generated by dummy phy we can send up to mac else nodeb should generate the "real" feedback
    if (lteinfo->feedbackReq.request)
    {
        requestFeedback(lteinfo, frame, pktAux);

        // DEBUG
        bool debug = false;
        if( debug )
        {
            LteFeedbackDoubleVector::iterator it;
            LteFeedbackVector::iterator jt;
            LteFeedbackDoubleVector vec = header->getLteFeedbackDoubleVectorDl();
            for (it = vec.begin(); it != vec.end(); ++it)
            {
                for (jt = it->begin(); jt != it->end(); ++jt)
                {
                    MacNodeId id = lteinfo->getSourceId();
                    EV << endl << "Node:" << id << endl;
                    TxMode t = jt->getTxMode();
                    EV << "TXMODE: " << txModeToA(t) << endl;
                    if (jt->hasBandCqi())
                    {
                        std::vector<CqiVector> vec = jt->getBandCqi();
                        std::vector<CqiVector>::iterator kt;
                        CqiVector::iterator ht;
                        int i;
                        for (kt = vec.begin(); kt != vec.end(); ++kt)
                        {
                            for (i = 0, ht = kt->begin(); ht != kt->end();
                                ++ht, i++)
                            EV << "Banda " << i << " Cqi " << *ht << endl;
                        }
                    }
                    else if (jt->hasWbCqi())
                    {
                        CqiVector v = jt->getWbCqi();
                        CqiVector::iterator ht = v.begin();
                        for (; ht != v.end(); ++ht)
                        EV << "wb cqi " << *ht << endl;
                    }
                    if (jt->hasRankIndicator())
                    {
                        EV << "Rank " << jt->getRankIndicator() << endl;
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
LteFeedbackComputation* LtePhyEnb::getFeedbackComputationFromName(
    std::string name, ParameterMap& params)
{
    ParameterMap::iterator it;
    if (name == "REAL")
    {
        //default value
        double targetBler = 0.1;
        double lambdaMinTh = 0.02;
        double lambdaMaxTh = 0.2;
        double lambdaRatioTh = 20;
        it = params.find("targetBler");
        if (it != params.end())
        {
            targetBler = params["targetBler"].doubleValue();
        }
        it = params.find("lambdaMinTh");
        if (it != params.end())
        {
            lambdaMinTh = params["lambdaMinTh"].doubleValue();
        }
        it = params.find("lambdaMaxTh");
        if (it != params.end())
        {
            lambdaMaxTh = params["lambdaMaxTh"].doubleValue();
        }
        it = params.find("lambdaRatioTh");
        if (it != params.end())
        {
            lambdaRatioTh = params["lambdaRatioTh"].doubleValue();
        }
        LteFeedbackComputation* fbcomp = new LteFeedbackComputationRealistic(
            targetBler, cellInfo_->getLambda(), lambdaMinTh, lambdaMaxTh,
            lambdaRatioTh, cellInfo_->getNumBands());
        return fbcomp;
    }
    else
        return 0;
}

void LtePhyEnb::initializeFeedbackComputation()
{
    lteFeedbackComputation_ = 0;

    const char* name = "REAL";

    double targetBler = par("targetBler");
    double lambdaMinTh = par("lambdaMinTh");
    double lambdaMaxTh = par("lambdaMaxTh");
    double lambdaRatioTh = par("lambdaRatioTh");

//    lteFeedbackComputation_ = new LteFeedbackComputationRealistic(
//        targetBler, cellInfo_->getLambda(), lambdaMinTh, lambdaMaxTh,
//        lambdaRatioTh, cellInfo_->getNumBands());

    // compute feedback for the primary carrier only
    // TODO add support for feedback computation for all carriers
    lteFeedbackComputation_ = new LteFeedbackComputationRealistic(
        targetBler, cellInfo_->getLambda(), lambdaMinTh, lambdaMaxTh,
        lambdaRatioTh, cellInfo_->getPrimaryCarrierNumBands());

    EV << "Feedback Computation \"" << name << "\" loaded." << endl;
}

