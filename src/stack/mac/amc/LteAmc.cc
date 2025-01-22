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
//
#include <omnetpp.h>

#include "stack/mac/amc/LteAmc.h"
#include "stack/mac/LteMacEnb.h"

// NOTE: AMC Pilots header file inclusions must go here
#include "stack/mac/amc/AmcPilotAuto.h"
#include "stack/mac/amc/AmcPilotD2D.h"

namespace simu5g {

using namespace omnetpp;
LteAmc::~LteAmc()
{
    delete pilot_;
}

/*********************
* PRIVATE FUNCTIONS
*********************/

AmcPilot *LteAmc::getAmcPilot(const cPar& p)
{
    EV << "Creating Amc pilot " << p.stringValue() << endl;
    const char *s = p.stringValue();
    if (strcmp(s, "AUTO") == 0)
        return new AmcPilotAuto(binder_, this);
    if (strcmp(s, "D2D") == 0)
        return new AmcPilotD2D(binder_, this);
    throw cRuntimeError("AMC Pilot not recognized");
}

MacNodeId LteAmc::getNextHop(MacNodeId dst)
{
    MacNodeId nh = binder_->getNextHop(dst);

    if (nh == nodeId_) {
        // I'm the master for this slave (it is directly connected)
        return dst;
    }

    EV << "LteAmc::getNextHop Node Id dst : " << dst << endl;

    return nh;
}

void LteAmc::printParameters()
{
    EV << "###################" << endl;
    EV << "# LteAmc parameters" << endl;
    EV << "###################" << endl;

    EV << "NumUeDl: " << dlConnectedUe_.size() << endl;
    EV << "NumUeUl: " << ulConnectedUe_.size() << endl;
    EV << "Number of cell bands: " << numBands_ << endl;

    EV << "MacNodeId: " << nodeId_ << endl;
    EV << "MacCellId: " << cellId_ << endl;
    EV << "AmcMode: " << mac_->par("amcMode").stdstringValue() << endl;
    EV << "RbAllocationType: " << allocationType_ << endl;
    EV << "FBHB capacity DL: " << fbhbCapacityDl_ << endl;
    EV << "FBHB capacity UL: " << fbhbCapacityUl_ << endl;
    EV << "PmiWeight: " << pmiComputationWeight_ << endl;
    EV << "CqiWeight: " << cqiComputationWeight_ << endl;
    EV << "DL MCS scale: " << mcsScaleDl_ << endl;
    EV << "UL MCS scale: " << mcsScaleUl_ << endl;
    EV << "Confidence LB: " << lb_ << endl;
    EV << "Confidence UB: " << ub_ << endl;
}

void LteAmc::printFbhb(Direction dir)
{
    EV << "###################################" << endl;
    EV << "# AMC Feedback Historical Base (" << dirToA(dir) << ")" << endl;
    EV << "###################################" << endl;

    std::map<double, History_> *history;
    std::vector<MacNodeId> *revIndex;

    if (dir == DL) {
        history = &dlFeedbackHistory_;
        revIndex = &dlRevNodeIndex_;
    }
    else if (dir == UL) {
        history = &ulFeedbackHistory_;
        revIndex = &ulRevNodeIndex_;
    }
    else {
        throw cRuntimeError("LteAmc::printFbhb(): Unrecognized direction");
    }

    for (auto& [carrier, hist] : *history) { // for each antenna
        EV << simTime() << " # Carrier: " << carrier << "\n";
        for (auto& histItem : hist) {
            EV << simTime() << " # Remote: " << dasToA(histItem.first) << "\n";
            for (size_t i = 0; i < histItem.second.size(); i++) { // for each UE
                EV << "Ue index: " << i << ", MacNodeId: " << (*revIndex)[i] << endl;
                auto& txVec = histItem.second[i];
                for (size_t t = 0; t < txVec.size(); t++) { // for each tx mode
                    TxMode txMode = TxMode(t);

                    // Print only non-empty feedback summary! (all cqi are != NOSIGNALCQI)
                    Cqi testCqi = txVec[t].get().getCqi(Codeword(0), Band(0));
                    if (testCqi == NOSIGNALCQI)
                        continue;

                    EV << "@TxMode " << txMode << endl;
                    txVec[t].get().print(NODEID_NONE, (*revIndex)[i], dir, txMode, "LteAmc::printAmcFbhb");
                }
            }
        }
    }
}

void LteAmc::printTxParams(Direction dir, double carrierFrequency)
{
    EV << "######################" << endl;
    EV << "# UserTxParams vector (" << dirToA(dir) << ")" << endl;
    EV << "######################" << endl;

    std::vector<UserTxParams> *userInfo;
    std::vector<MacNodeId> *revIndex;

    if (dir == DL) {
        userInfo = &dlTxParams_[carrierFrequency];
        revIndex = &dlRevNodeIndex_;
    }
    else if (dir == UL) {
        userInfo = &ulTxParams_[carrierFrequency];
        revIndex = &ulRevNodeIndex_;
    }
    else if (dir == D2D) {
        userInfo = &d2dTxParams_[carrierFrequency];
        revIndex = &d2dRevNodeIndex_;
    }
    else {
        throw cRuntimeError("LteAmc::printTxParams(): Unrecognized direction");
    }

    // Cqi testCqi=0;
    for (int index = 0; index < userInfo->size(); index++) {
        EV << "Ue index: " << index << ", MacNodeId: " << (*revIndex)[index] << endl;

        // Print only non-empty user transmission parameters
        // testCqi = userInfo->at(index).readCqiVector().at(0);
        //if(testCqi!=0)
        userInfo->at(index).print("info");
    }
}

void LteAmc::printMuMimoMatrix(const char *s)
{
    muMimoDlMatrix_.print(s);
    muMimoUlMatrix_.print(s);
    muMimoD2DMatrix_.print(s);
}

/********************
* PUBLIC FUNCTIONS
********************/

LteAmc::LteAmc(LteMacEnb *mac, Binder *binder, CellInfo *cellInfo, int numAntennas) : mac_(mac), binder_(binder), cellInfo_(cellInfo), numAntennas_(numAntennas)
{
    initialize();
}

LteAmc& LteAmc::operator=(const LteAmc& other)
{
    if (&other == this)
        return *this;

    mac_ = other.mac_;
    binder_ = other.binder_;
    cellInfo_ = other.cellInfo_;
    pilot_ = getAmcPilot(mac_->par("amcMode"));

    allocationType_ = other.allocationType_;
    numBands_ = other.numBands_;
    nodeId_ = other.nodeId_;
    cellId_ = other.cellId_;
    dlMcsTable_ = other.dlMcsTable_;
    ulMcsTable_ = other.ulMcsTable_;
    d2dMcsTable_ = other.d2dMcsTable_;
    mcsScaleDl_ = other.mcsScaleDl_;
    mcsScaleUl_ = other.mcsScaleUl_;
    mcsScaleD2D_ = other.mcsScaleD2D_;
    numAntennas_ = other.numAntennas_;
    remoteSet_ = other.remoteSet_;
    dlConnectedUe_ = other.dlConnectedUe_;
    ulConnectedUe_ = other.ulConnectedUe_;
    d2dConnectedUe_ = other.d2dConnectedUe_;
    dlNodeIndex_ = other.dlNodeIndex_;
    ulNodeIndex_ = other.ulNodeIndex_;
    d2dNodeIndex_ = other.d2dNodeIndex_;
    dlRevNodeIndex_ = other.dlRevNodeIndex_;
    ulRevNodeIndex_ = other.ulRevNodeIndex_;
    d2dRevNodeIndex_ = other.d2dRevNodeIndex_;

    dlTxParams_ = other.dlTxParams_;
    ulTxParams_ = other.ulTxParams_;
    d2dTxParams_ = other.d2dTxParams_;

    fType_ = other.fType_;

    dlFeedbackHistory_ = other.dlFeedbackHistory_;
    ulFeedbackHistory_ = other.ulFeedbackHistory_;
    d2dFeedbackHistory_ = other.d2dFeedbackHistory_;

    fbhbCapacityDl_ = other.fbhbCapacityDl_;
    fbhbCapacityUl_ = other.fbhbCapacityUl_;
    fbhbCapacityD2D_ = other.fbhbCapacityD2D_;
    lb_ = other.lb_;
    ub_ = other.ub_;
    pmiComputationWeight_ = other.pmiComputationWeight_;
    cqiComputationWeight_ = other.cqiComputationWeight_;
    muMimoDlMatrix_ = other.muMimoDlMatrix_;
    muMimoUlMatrix_ = other.muMimoUlMatrix_;
    muMimoD2DMatrix_ = other.muMimoD2DMatrix_;

    return *this;
}

void LteAmc::initialize()
{
    // Get MacNodeId and MacCellId
    nodeId_ = mac_->getMacNodeId();
    cellId_ = mac_->getMacCellId();

    // Get deployed UEs maps from Binder
    dlConnectedUe_ = binder_->getDeployedUes(nodeId_, DL);
    ulConnectedUe_ = binder_->getDeployedUes(nodeId_, UL);
    d2dConnectedUe_ = binder_->getDeployedUes(nodeId_, UL);

    // Get parameters from cellInfo
    numBands_ = cellInfo_->getPrimaryCarrierNumBands();
    mcsScaleDl_ = cellInfo_->getMcsScaleDl();
    mcsScaleUl_ = cellInfo_->getMcsScaleUl();
    mcsScaleD2D_ = cellInfo_->getMcsScaleUl();

    // Get AMC parameters from MAC module NED
    fbhbCapacityDl_ = mac_->par("fbhbCapacityDl");
    fbhbCapacityUl_ = mac_->par("fbhbCapacityUl");
    fbhbCapacityD2D_ = mac_->par("fbhbCapacityD2D");
    pmiComputationWeight_ = mac_->par("pmiWeight");
    cqiComputationWeight_ = mac_->par("cqiWeight");
    pilot_ = getAmcPilot(mac_->par("amcMode"));
    allocationType_ = getRbAllocationType(mac_->par("rbAllocationType").stringValue());
    lb_ = mac_->par("summaryLowerBound");
    ub_ = mac_->par("summaryUpperBound");

    printParameters();

    // Structures initialization

    // Scale MCS Tables
    dlMcsTable_.rescale(mcsScaleDl_);
    ulMcsTable_.rescale(mcsScaleUl_);
    d2dMcsTable_.rescale(mcsScaleD2D_);

    // Initialize DAS structures
    for (int i = 0; i < numAntennas_; i++) {
        EV << "Adding Antenna: " << dasToA(Remote(i)) << endl;
        remoteSet_.insert(Remote(i));
    }

    // Initializing feedback and scheduling structures

    /**
     * Preparing iterators.
     * Note: at initialization ALL dlConnectedUe_ and ulConnectedUe_ elements are TRUE.
     */
    ConnectedUesMap::const_iterator it, et;
    RemoteSet::const_iterator ait, aet;

    // DOWNLINK
    EV << "DL CONNECTED: " << dlConnectedUe_.size() << endl;

    for (auto [nodeId, flag] : dlConnectedUe_) { // For all UEs (DL)
        dlNodeIndex_[nodeId] = dlRevNodeIndex_.size();
        dlRevNodeIndex_.push_back(nodeId);

        EV << "Creating UE, id: " << nodeId << ", index: " << dlNodeIndex_[nodeId] << endl;
    }

    // UPLINK
    EV << "UL CONNECTED: " << dlConnectedUe_.size() << endl;

    for (auto [nodeId, flag] : ulConnectedUe_) { // For all UEs (UL)
        ulNodeIndex_[nodeId] = ulRevNodeIndex_.size();
        ulRevNodeIndex_.push_back(nodeId);
    }

    // D2D
    EV << "D2D CONNECTED: " << d2dConnectedUe_.size() << endl;

    for (auto [nodeId, flag] : d2dConnectedUe_) { // For all UEs (D2D)
        d2dNodeIndex_[nodeId] = d2dRevNodeIndex_.size();
        d2dRevNodeIndex_.push_back(nodeId);
    }

    //printFbhb(DL);
    //printFbhb(UL);
    //printTxParams(DL);
    //printTxParams(UL);
}

void LteAmc::rescaleMcs(double rePerRb, Direction dir)
{
    if (dir == DL) {
        dlMcsTable_.rescale(rePerRb);
    }
    else if (dir == UL) {
        ulMcsTable_.rescale(rePerRb);
    }
    else if (dir == D2D) {
        d2dMcsTable_.rescale(rePerRb);
    }
}

/*******************************************
*    Functions for feedback management    *
*******************************************/

History_ *LteAmc::getHistory(Direction dir, double carrierFrequency)
{
    History_ history;
    std::map<double, History_> *historyMap = (dir == DL) ? &dlFeedbackHistory_ : &ulFeedbackHistory_;
    if (historyMap->find(carrierFrequency) == historyMap->end()) {
        // initialize new entry

        ConnectedUesMap *connectedUe = (dir == DL) ? &dlConnectedUe_ : &ulConnectedUe_;
        const unsigned char num_tx_mode = (dir == DL) ? DL_NUM_TXMODE : UL_NUM_TXMODE;
        int fbhbCapacity = (dir == DL) ? fbhbCapacityDl_ : fbhbCapacityUl_;

        ConnectedUesMap::const_iterator it, et;
        RemoteSet::const_iterator ait, aet;

        it = connectedUe->begin();
        et = connectedUe->end();
        for ( ; it != et; it++) { // For all UEs (DL)
            for (auto remote : remoteSet_) {
                // initialize historical feedback base for this UE (index) for all tx modes and for all RUs
                history[remote].push_back(
                        std::vector<LteSummaryBuffer>(num_tx_mode,
                                LteSummaryBuffer(fbhbCapacity, MAXCW, numBands_, lb_, ub_)));
            }
        }
        (*historyMap)[carrierFrequency] = history;
    }
    return &(historyMap->at(carrierFrequency));
}

void LteAmc::pushFeedback(MacNodeId id, Direction dir, LteFeedback fb, double carrierFrequency)
{
    EV << "Feedback from MacNodeId " << id << " (direction " << dirToA(dir) << ")" << endl;

    History_ *history;
    std::map<MacNodeId, unsigned int> *nodeIndex;

    history = getHistory(dir, carrierFrequency);
    if (dir == DL) {
        nodeIndex = &dlNodeIndex_;
    }
    else if (dir == UL) {
        nodeIndex = &ulNodeIndex_;
    }
    else {
        throw cRuntimeError("LteAmc::pushFeedback(): Unrecognized direction");
    }

    // Put the feedback in the FBHB
    Remote antenna = fb.getAntennaId();
    TxMode txMode = fb.getTxMode();
    if (nodeIndex->find(id) == nodeIndex->end()) {
        return;
    }
    int index = (*nodeIndex).at(id);

    EV << "ID: " << id << endl;
    EV << "index: " << index << endl;
    (*history)[antenna].at(index).at(txMode).put(fb);

    // delete the old UserTxParam for this <UE_dir_carrierFreq>, so that it will be recomputed next time it's needed
    std::map<double, std::vector<UserTxParams>> *txParams = (dir == DL) ? &dlTxParams_ : (dir == UL) ? &ulTxParams_ : throw cRuntimeError("LteAmc::pushFeedback(): Unrecognized direction");
    if (txParams->find(carrierFrequency) != txParams->end() && txParams->at(carrierFrequency).at(index).isSet())
        (*txParams)[carrierFrequency].at(index).restoreDefaultValues();

    // DEBUG
    EV << "Antenna: " << dasToA(antenna) << ", TxMode: " << txMode << ", Index: " << index << endl;
    EV << "RECEIVED" << endl;
    fb.print(cellId_, id, dir, "LteAmc::pushFeedback");
}

void LteAmc::pushFeedbackD2D(MacNodeId id, LteFeedback fb, MacNodeId peerId, double carrierFrequency)
{
    EV << "Feedback from MacNodeId " << id << " (direction D2D), peerId = " << peerId << endl;

    std::map<MacNodeId, History_> *history = &d2dFeedbackHistory_[carrierFrequency];
    std::map<MacNodeId, unsigned int> *nodeIndex = &d2dNodeIndex_;

    // Put the feedback in the FBHB
    Remote antenna = fb.getAntennaId();
    TxMode txMode = fb.getTxMode();
    int index = (*nodeIndex).at(id);

    EV << "ID: " << id << endl;
    EV << "index: " << index << endl;

    if (history->find(peerId) == history->end()) {
        // initialize new history for this peer UE
        History_ newHist;

        ConnectedUesMap::const_iterator it, et;
        it = d2dConnectedUe_.begin();
        et = d2dConnectedUe_.end();
        for ( ; it != et; it++) { // For all UEs (D2D)
            newHist[antenna].push_back(std::vector<LteSummaryBuffer>(UL_NUM_TXMODE, LteSummaryBuffer(fbhbCapacityD2D_, MAXCW, numBands_, lb_, ub_)));
        }
        (*history)[peerId] = newHist;
    }
    (*history)[peerId][antenna].at(index).at(txMode).put(fb);

    // delete the old UserTxParam for this <UE_dir_carrierFreq>, so that it will be recomputed next time it's needed
    if (d2dTxParams_.find(carrierFrequency) != d2dTxParams_.end() && d2dTxParams_.at(carrierFrequency).at(index).isSet())
        d2dTxParams_[carrierFrequency].at(index).restoreDefaultValues();

    // DEBUG
    EV << "PeerId: " << peerId << ", Antenna: " << dasToA(antenna) << ", TxMode: " << txMode << ", Index: " << index << endl;
    EV << "RECEIVED" << endl;
    fb.print(NODEID_NONE, id, D2D, "LteAmc::pushFeedbackD2D");
}

const LteSummaryFeedback& LteAmc::getFeedback(MacNodeId id, Remote antenna, TxMode txMode, const Direction dir, double carrierFrequency)
{
    MacNodeId nh = getNextHop(id);
    if (id != nh)
        EV << NOW << " LteAmc::getFeedback detected " << nh << " as next hop for " << id << "\n";
    id = nh;

    if (dir != DL && dir != UL)
        throw cRuntimeError("LteAmc::getFeedback(): Unrecognized direction");

    History_ *history = getHistory(dir, carrierFrequency);
    std::map<MacNodeId, unsigned int> *nodeIndex = (dir == DL) ? &dlNodeIndex_ : &ulNodeIndex_;

    return (*history).at(antenna).at((*nodeIndex).at(id)).at(txMode).get();
}

const LteSummaryFeedback& LteAmc::getFeedbackD2D(MacNodeId id, Remote antenna, TxMode txMode, MacNodeId peerId, double carrierFrequency)
{
    MacNodeId nh = getNextHop(id);

    if (id != nh)
        EV << NOW << " LteAmc::getFeedbackD2D detected " << nh << " as next hop for " << id << "\n";
    id = nh;

    if (peerId == NODEID_NONE) {
        // we return the first feedback stored in the structure
        for (const auto& [histNodeId, history] : d2dFeedbackHistory_.at(carrierFrequency)) {
            if (histNodeId == NODEID_NONE) // skip fake UE 0
                continue;

            if (binder_->getD2DCapability(id, histNodeId)) {
                peerId = histNodeId;
                break;
            }
        }

        // default feedback: when there is no feedback from peers yet (NOSIGNALCQI)
        if (peerId == NODEID_NONE)
            return d2dFeedbackHistory_.at(carrierFrequency).at(NODEID_NONE).at(MACRO).at(0).at(txMode).get();
    }
    return d2dFeedbackHistory_.at(carrierFrequency).at(peerId).at(antenna).at(d2dNodeIndex_.at(id)).at(txMode).get();
}

/*******************************************
*    Functions for MU-MIMO support       *
*******************************************/

MacNodeId LteAmc::computeMuMimoPairing(const MacNodeId nodeId, Direction dir)
{
    if (dir == DL) {
        return muMimoDlMatrix_.getMuMimoPair(nodeId);
    }
    else if (dir == UL) {
        return muMimoUlMatrix_.getMuMimoPair(nodeId);
    }
    else if (dir == D2D) {
        return muMimoD2DMatrix_.getMuMimoPair(nodeId);
    }
    throw cRuntimeError("LteAmc::computeMuMimoPairing(): Unrecognized direction");
}

/********************************
*       Access Functions       *
********************************/

bool LteAmc::existTxParams(MacNodeId id, const Direction dir, double carrierFrequency)
{
    MacNodeId nh = getNextHop(id);
    if (id != nh)
        EV << NOW << " LteAmc::existTxParams detected " << nh << " as next hop for " << id << "\n";
    id = nh;

    std::map<double, std::vector<UserTxParams>> *txParams = (dir == DL) ? &dlTxParams_ : (dir == UL) ? &ulTxParams_ : (dir == D2D) ? &d2dTxParams_ : throw cRuntimeError("LteAmc::existTxParams(): Unrecognized direction");
    if (txParams->find(carrierFrequency) == txParams->end())
        return false;

    std::map<MacNodeId, unsigned int>& nodeIndex = (dir == DL) ? dlNodeIndex_ : (dir == UL) ? ulNodeIndex_ : d2dNodeIndex_;

    return (*txParams)[carrierFrequency].at(nodeIndex.at(id)).isSet();
}

const UserTxParams& LteAmc::setTxParams(MacNodeId id, const Direction dir, UserTxParams& info, double carrierFrequency)
{
    MacNodeId nh = getNextHop(id);
    if (id != nh)
        EV << NOW << " LteAmc::setTxParams detected " << nh << " as next hop for " << id << "\n";
    id = nh;

    info.isSet() = true;

    /**
     * NOTE: if the antenna set has not been explicitly written in UserTxParams
     * by the AMC pilot, this antenna set contains only MACRO
     * (this is done by setting MACRO in UserTxParams constructor)
     */

    // DEBUG
    EV << NOW << " LteAmc::setTxParams DAS antenna set for user " << id << " is \t";
    for (auto it : info.readAntennaSet()) {
        EV << "[" << dasToA(it) << "]\t";
    }
    EV << endl;

    std::map<double, std::vector<UserTxParams>> *txParams = (dir == DL) ? &dlTxParams_ : (dir == UL) ? &ulTxParams_ : (dir == D2D) ? &d2dTxParams_ : throw cRuntimeError("LteAmc::setTxParams(): Unrecognized direction");
    std::map<MacNodeId, unsigned int>& nodeIndex = (dir == DL) ? dlNodeIndex_ : (dir == UL) ? ulNodeIndex_ : d2dNodeIndex_;
    if (txParams->find(carrierFrequency) == txParams->end()) {
        // Initialize user transmission parameters structures
        ConnectedUesMap& connectedUe = (dir == DL) ? dlConnectedUe_ : ulConnectedUe_;
        std::vector<UserTxParams> tmp;
        tmp.resize(connectedUe.size(), UserTxParams());
        (*txParams)[carrierFrequency] = tmp;
    }
    return (*txParams)[carrierFrequency].at(nodeIndex.at(id)) = info;
}

const UserTxParams& LteAmc::computeTxParams(MacNodeId id, const Direction dir, double carrierFrequency)
{
    // DEBUG
    EV << NOW << " LteAmc::computeTxParams --------------::[ START ]::--------------\n";
    EV << NOW << " LteAmc::computeTxParams CellId: " << cellId_ << "\n";
    EV << NOW << " LteAmc::computeTxParams NodeId: " << id << "\n";
    EV << NOW << " LteAmc::computeTxParams Direction: " << dirToA(dir) << "\n";
    EV << NOW << " LteAmc::computeTxParams - - - - - - - - - - - - - - - - - - - - -\n";
    EV << NOW << " LteAmc::computeTxParams RB allocation type: " << allocationTypeToA(allocationType_) << "\n";
    EV << NOW << " LteAmc::computeTxParams - - - - - - - - - - - - - - - - - - - - -\n";

    MacNodeId nh = getNextHop(id);
    if (id != nh)
        EV << NOW << " LteAmc::computeTxParams detected " << nh << " as next hop for " << id << "\n";
    id = nh;

    const UserTxParams& info = pilot_->computeTxParams(id, dir, carrierFrequency);
    EV << NOW << " LteAmc::computeTxParams --------------::[  END  ]::--------------\n";

    return info;
}

/*******************************************
*      Scheduler interface functions      *
*******************************************/

unsigned int LteAmc::computeBitsOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency)
{
    if (blocks > 110)                          // Safety check to avoid segmentation fault
        throw cRuntimeError("LteAmc::computeBitsOnNRbs(): Too many blocks");

    if (blocks == 0)
        return 0;

    // DEBUG
    EV << NOW << " LteAmc::blocks2bits Node: " << id << "\n";
    EV << NOW << " LteAmc::blocks2bits Band: " << b << "\n";
    EV << NOW << " LteAmc::blocks2bits Direction: " << dirToA(dir) << "\n";

    // Acquiring current user scheduling information
    const UserTxParams& info = computeTxParams(id, dir, carrierFrequency);

    std::vector<unsigned char> layers = info.getLayers();

    unsigned int bits = 0;
    unsigned int codewords = layers.size();
    for (Codeword cw = 0; cw < codewords; ++cw) {
        // if CQI == 0 the UE is out of range, thus bits=0
        if (info.readCqiVector().at(cw) == 0) {
            EV << NOW << " LteAmc::blocks2bits - CQI equal to zero on cw " << cw << ", return no blocks available" << endl;
            continue;
        }

        LteMod mod = info.getCwModulation(cw);
        unsigned int iTbs = getItbsPerCqi(info.readCqiVector().at(cw), dir);
        unsigned int i = (mod == _QPSK ? 0 : (mod == _16QAM ? 9 : (mod == _64QAM ? 15 : 0)));

        // DEBUG
        EV << NOW << " LteAmc::blocks2bits ---::[ Codeword = " << cw << "\n";
        EV << NOW << " LteAmc::blocks2bits Modulation: " << modToA(mod) << "\n";
        EV << NOW << " LteAmc::blocks2bits iTbs: " << iTbs << "\n";
        EV << NOW << " LteAmc::blocks2bits i: " << i << "\n";
        EV << NOW << " LteAmc::blocks2bits CQI: " << info.readCqiVector().at(cw) << "\n";

        const unsigned int *tbsVect = itbs2tbs(mod, info.readTxMode(), layers.at(cw), iTbs - i);
        bits += tbsVect[blocks - 1];
    }

    // DEBUG
    EV << NOW << " LteAmc::blocks2bits Resource Blocks: " << blocks << "\n";
    EV << NOW << " LteAmc::blocks2bits Available space: " << bits << "\n";

    return bits;
}

unsigned int LteAmc::computeBitsOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir, double carrierFrequency)
{
    if (blocks > 110)                          // Safety check to avoid segmentation fault
        throw cRuntimeError("LteAmc::blocks2bits(): Too many blocks");

    if (blocks == 0)
        return 0;

    // DEBUG
    EV << NOW << " LteAmc::blocks2bits Node: " << id << "\n";
    EV << NOW << " LteAmc::blocks2bits Band: " << b << "\n";
    EV << NOW << " LteAmc::blocks2bits Codeword: " << cw << "\n";
    EV << NOW << " LteAmc::blocks2bits Direction: " << dirToA(dir) << "\n";

    // Acquiring current user scheduling information
    UserTxParams info = computeTxParams(id, dir, carrierFrequency);

    // if CQI == 0 the UE is out of range, thus return 0
    if (info.readCqiVector().at(cw) == 0) {
        EV << NOW << " LteAmc::blocks2bits - CQI equal to zero, return no blocks available" << endl;
        return 0;
    }
    unsigned char layers = info.getLayers().at(cw);

    unsigned int iTbs = getItbsPerCqi(info.readCqiVector().at(cw), dir);
    LteMod mod = info.getCwModulation(cw);
    unsigned int i = (mod == _QPSK ? 0 : (mod == _16QAM ? 9 : (mod == _64QAM ? 15 : 0)));

    // DEBUG
    EV << NOW << " LteAmc::blocks2bits Modulation: " << modToA(mod) << "\n";
    EV << NOW << " LteAmc::blocks2bits iTbs: " << iTbs << "\n";
    EV << NOW << " LteAmc::blocks2bits i: " << i << "\n";

    const unsigned int *tbsVect = itbs2tbs(mod, info.readTxMode(), layers, iTbs - i);

    // DEBUG
    EV << NOW << " LteAmc::blocks2bits Resource Blocks: " << blocks << "\n";
    EV << NOW << " LteAmc::blocks2bits Available space: " << tbsVect[blocks - 1] << "\n";

    return tbsVect[blocks - 1];
}

unsigned int LteAmc::computeBytesOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency)
{
    EV << NOW << " LteAmc::blocks2bytes Node " << id << ", Band " << b << ", direction " << dirToA(dir) << ", blocks " << blocks << "\n";

    unsigned int bits = computeBitsOnNRbs(id, b, blocks, dir, carrierFrequency);
    unsigned int bytes = bits / 8;

    // DEBUG
    EV << NOW << " LteAmc::blocks2bytes Resource Blocks: " << blocks << "\n";
    EV << NOW << " LteAmc::blocks2bytes Available space: " << bits << "\n";
    EV << NOW << " LteAmc::blocks2bytes Available space: " << bytes << "\n";

    return bytes;
}

unsigned int LteAmc::computeBytesOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir, double carrierFrequency)
{
    EV << NOW << " LteAmc::blocks2bytes Node " << id << ", Band " << b << ", Codeword " << cw << ", direction " << dirToA(dir) << ", blocks " << blocks << "\n";

    unsigned int bits = computeBitsOnNRbs(id, b, cw, blocks, dir, carrierFrequency);
    unsigned int bytes = bits / 8;

    // DEBUG
    EV << NOW << " LteAmc::blocks2bytes Resource Blocks: " << blocks << "\n";
    EV << NOW << " LteAmc::blocks2bytes Available space: " << bits << "\n";
    EV << NOW << " LteAmc::blocks2bytes Available space: " << bytes << "\n";

    return bytes;
}

unsigned int LteAmc::computeBytesOnNRbs_MB(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency)
{
    EV << NOW << " LteAmc::computeBytesOnNRbs_MB Node " << id << ", Band " << b << ", direction " << dirToA(dir) << ", blocks " << blocks << "\n";

    unsigned int bits = computeBitsOnNRbs_MB(id, b, blocks, dir, carrierFrequency);
    unsigned int bytes = bits / 8;

    // DEBUG
    EV << NOW << " LteAmc::computeBytesOnNRbs_MB Resource Blocks: " << blocks << "\n";
    EV << NOW << " LteAmc::computeBytesOnNRbs_MB Available space: " << bits << "\n";
    EV << NOW << " LteAmc::computeBytesOnNRbs_MB Available space: " << bytes << "\n";

    return bytes;
}

unsigned int LteAmc::computeBitsOnNRbs_MB(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency)
{
    if (blocks > 110)                          // Safety check to avoid segmentation fault
        throw cRuntimeError("LteAmc::computeBitsOnNRbs_MB(): Too many blocks");

    if (blocks == 0)
        return 0;

    // DEBUG
    EV << NOW << " LteAmc::computeBitsOnNRbs_MB Node: " << id << "\n";
    EV << NOW << " LteAmc::computeBitsOnNRbs_MB Band: " << b << "\n";
    EV << NOW << " LteAmc::computeBitsOnNRbs_MB Direction: " << dirToA(dir) << "\n";

    Cqi cqi = readMultiBandCqi(id, dir, carrierFrequency)[b];

    // Acquiring current user scheduling information
    UserTxParams info = computeTxParams(id, dir, carrierFrequency);

    std::vector<unsigned char> layers = info.getLayers();

    // if CQI == 0 the UE is out of range, thus return 0
    if (cqi == 0) {
        EV << NOW << " LteAmc::computeBitsOnNRbs_MB - CQI equal to zero, return no blocks available" << endl;
        return 0;
    }

    unsigned int iTbs = getItbsPerCqi(cqi, dir);
    LteMod mod = cqiTable[cqi].mod_;
    unsigned int i = (mod == _QPSK ? 0 : (mod == _16QAM ? 9 : (mod == _64QAM ? 15 : 0)));

    // DEBUG
    EV << NOW << " LteAmc::computeBitsOnNRbs_MB Modulation: " << modToA(mod) << "\n";
    EV << NOW << " LteAmc::computeBitsOnNRbs_MB iTbs: " << iTbs << "\n";
    EV << NOW << " LteAmc::computeBitsOnNRbs_MB i: " << i << "\n";

    const unsigned int *tbsVect = itbs2tbs(mod, TRANSMIT_DIVERSITY, layers[0], iTbs - i);

    // DEBUG
    EV << NOW << " LteAmc::computeBitsOnNRbs_MB Resource Blocks: " << blocks << "\n";
    EV << NOW << " LteAmc::computeBitsOnNRbs_MB Available space: " << tbsVect[blocks - 1] << "\n";

    return tbsVect[blocks - 1];
}

unsigned int LteAmc::computeBitsPerRbBackground(Cqi cqi, const Direction dir, double carrierFrequency)
{
    // DEBUG
    EV << NOW << " LteAmc::computeBitsPerRbBackground CQI: " << cqi << " Direction: " << dirToA(dir) << endl;

    // if CQI == 0 the UE is out of range, thus return 0
    if (cqi == 0) {
        EV << NOW << " LteAmc::computeBitsPerRbBackground - CQI equal to zero, return no bytes available" << endl;
        return 0;
    }

    unsigned int iTbs = getItbsPerCqi(cqi, dir);
    LteMod mod = cqiTable[cqi].mod_;
    unsigned int i = (mod == _QPSK ? 0 : (mod == _16QAM ? 9 : (mod == _64QAM ? 15 : 0)));

    EV << NOW << " LteAmc::computeBitsPerRbBackground Modulation: " << modToA(mod) << " - iTbs: " << iTbs << " i: " << i << endl;

    unsigned char layers = 1;
    const unsigned int *tbsVect = itbs2tbs(mod, TRANSMIT_DIVERSITY, layers, iTbs - i);
    unsigned int blocks = 1;

    EV << NOW << " LteAmc::computeBitsPerRbBackground Available space: " << tbsVect[blocks - 1] << "\n";
    return tbsVect[blocks - 1];
}

bool LteAmc::setPilotUsableBands(MacNodeId id, std::vector<unsigned short> usableBands)
{
    pilot_->setUsableBands(id, usableBands);
    return true;
}

UsableBands *LteAmc::getPilotUsableBands(MacNodeId id)
{
    return pilot_->getUsableBands(id);
}

unsigned int LteAmc::getItbsPerCqi(Cqi cqi, const Direction dir)
{
    // CQI threshold table selection
    McsTable *mcsTable;
    if (dir == DL)
        mcsTable = &dlMcsTable_;
    else if ((dir == UL) || (dir == D2D) || (dir == D2D_MULTI))
        mcsTable = &ulMcsTable_;
    else {
        throw cRuntimeError("LteAmc::getItbsPerCqi(): Unrecognized direction");
    }
    CQIelem entry = cqiTable[cqi];
    LteMod mod = entry.mod_;
    double rate = entry.rate_;

    // Select the ranges for searching in the McsTable.
    unsigned int min = 0; // _QPSK
    unsigned int max = 9; // _QPSK
    if (mod == _16QAM) {
        min = 10;
        max = 16;
    }
    if (mod == _64QAM) {
        min = 17;
        max = 28;
    }

    // Initialize the working variables at the minimum value.
    MCSelem elem = mcsTable->at(min);
    unsigned int iTbs = elem.iTbs_;

    // Search in the McsTable from min to max until the rate exceeds
    // the threshold in an entry of the table.
    for (unsigned int i = min; i <= max; i++) {
        elem = mcsTable->at(i);
        if (elem.threshold_ <= rate)
            iTbs = elem.iTbs_;
        else
            break;
    }

    // Return the iTbs found.
    return iTbs;
}

const UserTxParams& LteAmc::getTxParams(MacNodeId id, const Direction dir, double carrierFrequency)
{
    MacNodeId nh = getNextHop(id);
    if (id != nh)
        EV << NOW << " LteAmc::getTxParams detected " << nh << " as next hop for " << id << "\n";
    id = nh;

    if (dir == DL)
        return dlTxParams_[carrierFrequency].at(dlNodeIndex_.at(id));
    else if (dir == UL)
        return ulTxParams_[carrierFrequency].at(ulNodeIndex_.at(id));
    else if (dir == D2D)
        return d2dTxParams_[carrierFrequency].at(d2dNodeIndex_.at(id));
    else
        throw cRuntimeError("LteAmc::getTxParams(): Unrecognized direction");
}

unsigned int LteAmc::blockGain(Cqi cqi, unsigned int layers, unsigned int blocks, Direction dir)
{
    if (blocks > 110)                        // Safety check to avoid segmentation fault
        throw cRuntimeError("LteAmc::blocksGain(): Too many blocks (%d)", blocks);

    if (cqi > 15)                    // Safety check to avoid segmentation fault
        throw cRuntimeError("LteAmc::blocksGain(): CQI greater than 15 (%d)", cqi);

    if (blocks == 0)
        return 0;
    const unsigned int *tbsVect = readTbsVect(cqi, layers, dir);

    if (tbsVect == nullptr)
        return 0;
    return tbsVect[blocks - 1] / 8;
}

unsigned int LteAmc::bytesGain(Cqi cqi, unsigned int layers, unsigned int bytes, Direction dir)
{
    if (bytes == 0)
        return 0;
    const unsigned int *tbsVect = readTbsVect(cqi, layers, dir);

    if (tbsVect == nullptr)
        return 0;

    unsigned int i;
    for (i = 0; i < 110; ++i) {
        if (tbsVect[i] >= (bytes * 8))
            break;
    }
    return i + 1;
}

const unsigned int *LteAmc::readTbsVect(Cqi cqi, unsigned int layers, Direction dir)
{
    unsigned int itbs = getItbsPerCqi(cqi, dir);
    LteMod mod = cqiTable[cqi].mod_;

    const unsigned int *tbsVect = nullptr;
    switch (mod) {
        case _QPSK: {
            switch (layers) {
                case 1:
                    tbsVect = itbs2tbs_qpsk_1[itbs];
                    break;
                case 2:
                    tbsVect = itbs2tbs_qpsk_2[itbs];
                    break;
                case 4:
                    tbsVect = itbs2tbs_qpsk_4[itbs];
                    break;
                case 8:
                    tbsVect = itbs2tbs_qpsk_8[itbs];
                    break;
            }
            break;
        }
        case _16QAM: {
            switch (layers) {
                case 1:
                    tbsVect = itbs2tbs_16qam_1[itbs - 9];
                    break;
                case 2:
                    tbsVect = itbs2tbs_16qam_2[itbs - 9];
                    break;
                case 4:
                    tbsVect = itbs2tbs_16qam_4[itbs - 9];
                    break;
            }
            break;
        }
        case _64QAM: {
            switch (layers) {
                case 1:
                    tbsVect = itbs2tbs_64qam_1[itbs - 15];
                    break;
                case 2:
                    tbsVect = itbs2tbs_64qam_2[itbs - 15];
                    break;
                case 4:
                    tbsVect = itbs2tbs_64qam_4[itbs - 15];
                    break;
            }
            break;
        }
        case _256QAM: {
            throw cRuntimeError("LteAmc::readTbsVect(): Modulation _256QAM not handled.");
        }
    }
    return tbsVect;
}

/*************************************************
*    Functions for wideband values generation   *
*************************************************/

void LteAmc::writeCqiWeight(const double weight)
{
    // set the CQI weight
    cqiComputationWeight_ = weight;
}

Cqi LteAmc::readWbCqi(const CqiVector& cqi)
{
    // during the process, consider
    // - the cqi value which will be returned
    Cqi cqiRet = NOSIGNALCQI;
    // - a counter to obtain the mean
    Cqi cqiCounter = NOSIGNALCQI;
    // - the mean value
    Cqi cqiMean = NOSIGNALCQI;
    // - the min value
    Cqi cqiMin = NOSIGNALCQI;
    // - the max value
    Cqi cqiMax = NOSIGNALCQI;

    // consider the cqi of each band
    unsigned int bands = cqi.size();
    for (Band b = 0; b < bands; ++b) {
        EV << "LteAmc::getWbCqi - Cqi " << cqi.at(b) << " on band " << (int)b << endl;

        cqiCounter += cqi.at(b);
        cqiMin = cqiMin < cqi.at(b) ? cqiMin : cqi.at(b);
        cqiMax = cqiMax > cqi.at(b) ? cqiMax : cqi.at(b);
    }

    // when casting a double to an unsigned int value, consider the closest one

    // is the module lower than the half of the divisor? ceil, otherwise floor
    cqiMean = (double)(cqiCounter % bands) > (double)bands / 2.0 ? cqiCounter / bands + 1 : cqiCounter / bands;

    EV << "LteAmc::getWbCqi - Cqi mean " << cqiMean << " minimum " << cqiMin << " maximum " << cqiMax << endl;

    // the 0.0 weight is used in order to obtain the mean
    if (cqiComputationWeight_ == 0.0)
        cqiRet = cqiMean;
    // the -1.0 weight is used in order to obtain the min
    else if (cqiComputationWeight_ == -1.0)
        cqiRet = cqiMin;
    // the 1.0 weight is used in order to obtain the max
    else if (cqiComputationWeight_ == 1.0)
        cqiRet = cqiMax;
    // the following weight is used in order to obtain a value between the min and the mean
    else if (-1.0 < cqiComputationWeight_ && cqiComputationWeight_ < 0.0) {
        cqiRet = cqiMin;
        // ceil or floor depending on decimal part (casting to unsigned int results in a ceiling)
        double ret = (cqiComputationWeight_ + 1.0) * ((double)cqiMean - (double)cqiMin);
        cqiRet += ret - ((unsigned int)ret) > 0.5 ? (unsigned int)ret + 1 : (unsigned int)ret;
    }
    // the following weight is used in order to obtain a value between the min and the max
    else if (0.0 < cqiComputationWeight_ && cqiComputationWeight_ < 1.0) {
        cqiRet = cqiMean;
        // ceil or floor depending on decimal part (casting to unsigned int results in a ceiling)
        double ret = (cqiComputationWeight_) * ((double)cqiMax - (double)cqiMean);
        cqiRet += ret - ((unsigned int)ret) > 0.5 ? (unsigned int)ret + 1 : (unsigned int)ret;
    }
    else {
        throw cRuntimeError("LteAmc::getWbCqi(): Unknown weight %f", cqiComputationWeight_);
    }

    EV << "LteAmc::getWbCqi - Cqi " << cqiRet << " evaluated\n";

    return cqiRet;
}

std::vector<Cqi> LteAmc::readMultiBandCqi(MacNodeId id, const Direction dir, double carrierFrequency)
{
    return pilot_->getMultiBandCqi(id, dir, carrierFrequency);
}

void LteAmc::writePmiWeight(const double weight)
{
    // set the PMI weight
    pmiComputationWeight_ = weight;
}

Pmi LteAmc::readWbPmi(const PmiVector& pmi)
{
    // during the process, consider
    // - the pmi value which will be returned
    Pmi pmiRet = NOPMI;
    // - a counter to obtain the mean
    Pmi pmiCounter = NOPMI;
    // - the mean value
    Pmi pmiMean = NOPMI;
    // - the min value
    Pmi pmiMin = NOPMI;
    // - the max value
    Pmi pmiMax = NOPMI;

    // consider the pmi of each band
    unsigned int bands = pmi.size();
    for (Band b = 0; b < bands; ++b) {
        pmiCounter += pmi.at(b);
        pmiMin = pmiMin < pmi.at(b) ? pmiMin : pmi.at(b);
        pmiMax = pmiMax > pmi.at(b) ? pmiMax : pmi.at(b);
    }

    // when casting a double to an unsigned int value, consider the closest one

    // is the module lower than the half of the divisor? ceil, otherwise floor
    pmiMean = (double)(pmiCounter % bands) > (double)bands / 2.0 ? pmiCounter / bands + 1 : pmiCounter / bands;

    // the 0.0 weight is used in order to obtain the mean
    if (pmiComputationWeight_ == 0.0)
        pmiRet = pmiMean;
    // the -1.0 weight is used in order to obtain the min
    else if (pmiComputationWeight_ == -1.0)
        pmiRet = pmiMin;
    // the 1.0 weight is used in order to obtain the max
    else if (pmiComputationWeight_ == 1.0)
        pmiRet = pmiMax;
    // the following weight is used in order to obtain a value between the min and the mean
    else if (-1.0 < pmiComputationWeight_ && pmiComputationWeight_ < 0.0) {
        pmiRet = pmiMin;
        // ceil or floor depending on decimal part (casting to unsigned int results in a ceiling)
        double ret = (pmiComputationWeight_ + 1.0) * ((double)pmiMean - (double)pmiMin);
        pmiRet += ret - ((unsigned int)ret) > 0.5 ? (unsigned int)ret + 1 : (unsigned int)ret;
    }
    // the following weight is used in order to obtain a value between the min and the max
    else if (0.0 < pmiComputationWeight_ && pmiComputationWeight_ < 1.0) {
        pmiRet = pmiMean;
        // ceil or floor depending on decimal part (casting to unsigned int results in a ceiling)
        double ret = (pmiComputationWeight_) * ((double)pmiMax - (double)pmiMean);
        pmiRet += ret - ((unsigned int)ret) > 0.5 ? (unsigned int)ret + 1 : (unsigned int)ret;
    }
    else {
        throw cRuntimeError("LteAmc::readWbPmi(): Unknown weight %f", pmiComputationWeight_);
    }

    EV << "LteAmc::getWbPmi - Pmi " << pmiRet << " evaluated\n";

    return pmiRet;
}

/****************************
*    Handover support
****************************/

void LteAmc::detachUser(MacNodeId nodeId, Direction dir)
{
    EV << "##################################" << endl;
    EV << "# LteAmc::detachUser. Id: " << nodeId << ", direction: " << dirToA(dir) << endl;
    EV << "##################################" << endl;
    try {
        ConnectedUesMap *connectedUe;
        std::map<double, std::vector<UserTxParams>> *userInfoVec;
        std::map<double, History_> *history;
        std::map<double, std::map<MacNodeId, History_>> *d2dHistory;
        unsigned int nodeIndex;

        if (dir == DL) {
            connectedUe = &dlConnectedUe_;
            userInfoVec = &dlTxParams_;
            history = &dlFeedbackHistory_;
            nodeIndex = dlNodeIndex_.at(nodeId);
        }
        else if (dir == UL) {
            connectedUe = &ulConnectedUe_;
            userInfoVec = &ulTxParams_;
            history = &ulFeedbackHistory_;
            nodeIndex = ulNodeIndex_.at(nodeId);
        }
        else if (dir == D2D) {
            connectedUe = &d2dConnectedUe_;
            userInfoVec = &d2dTxParams_;
            d2dHistory = &d2dFeedbackHistory_;
            nodeIndex = d2dNodeIndex_.at(nodeId);
        }
        else {
            throw cRuntimeError("LteAmc::detachUser(): Unrecognized direction");
        }
        // UE is no longer connected
        (*connectedUe).at(nodeId) = false;

        // clear feedback data from history
        if (dir == UL || dir == DL) {
            for (auto& hit : *history) {
                for (auto remote : remoteSet_) {
                    hit.second.at(remote).at(nodeIndex).clear();
                }
            }
        }
        else { // D2D
            for (auto& hit : *d2dHistory) {
                for (auto& ht : hit.second) {
                    if (ht.first == NODEID_NONE)                                          // skip fake UE 0
                        continue;

                    for (auto remote : remoteSet_) {
                        ht.second.at(remote).at(nodeIndex).clear();
                    }
                }
            }
        }

        // clear user transmission parameters for this UE
        for (auto& item : *userInfoVec) {
            item.second.at(nodeIndex).restoreDefaultValues();
        }
    }
    catch (std::exception& e) {
        throw cRuntimeError("Exception in LteAmc::detachUser(): %s", e.what());
    }
}

void LteAmc::attachUser(MacNodeId nodeId, Direction dir)
{
    EV << "##################################" << endl;
    EV << "# LteAmc::attachUser. Id: " << nodeId << ", direction: " << dirToA(dir) << endl;
    EV << "##################################" << endl;

    ConnectedUesMap *connectedUe;
    std::map<MacNodeId, unsigned int> *nodeIndexMap;
    std::vector<MacNodeId> *revIndexVec;
    std::map<double, std::vector<UserTxParams>> *userInfoVec;
    std::map<double, History_> *history;
    std::map<double, std::map<MacNodeId, History_>> *d2dHistory;
    unsigned int nodeIndex;
    unsigned int fbhbCapacity;
    unsigned int numTxModes;

    if (dir == DL) {
        connectedUe = &dlConnectedUe_;
        nodeIndexMap = &dlNodeIndex_;
        revIndexVec = &dlRevNodeIndex_;
        userInfoVec = &dlTxParams_;
        history = &dlFeedbackHistory_;
        fbhbCapacity = fbhbCapacityDl_;
        numTxModes = DL_NUM_TXMODE;
    }
    else if (dir == UL) {
        connectedUe = &ulConnectedUe_;
        nodeIndexMap = &ulNodeIndex_;
        revIndexVec = &ulRevNodeIndex_;
        userInfoVec = &ulTxParams_;
        history = &ulFeedbackHistory_;
        fbhbCapacity = fbhbCapacityUl_;
        numTxModes = UL_NUM_TXMODE;
    }
    else if (dir == D2D) {
        connectedUe = &d2dConnectedUe_;
        nodeIndexMap = &d2dNodeIndex_;
        revIndexVec = &d2dRevNodeIndex_;
        userInfoVec = &d2dTxParams_;
        d2dHistory = &d2dFeedbackHistory_;
        fbhbCapacity = fbhbCapacityD2D_;
        numTxModes = UL_NUM_TXMODE;
    }
    else {
        throw cRuntimeError("LteAmc::attachUser(): Unrecognized direction");
    }

    // Prepare iterators and empty feedback data
    LteSummaryBuffer b = LteSummaryBuffer(fbhbCapacity, MAXCW, numBands_, lb_, ub_);
    std::vector<LteSummaryBuffer> v = std::vector<LteSummaryBuffer>(numTxModes, b);

    // check if the UE is known (it has been here before)
    if ((*connectedUe).find(nodeId) != (*connectedUe).end()) {
        EV << "LteAmc::attachUser. Id " << nodeId << " is known (he has been here before)." << endl;

        // user is known, get his index
        nodeIndex = (*nodeIndexMap).at(nodeId);

        // clear user transmission parameters for this UE
        for (auto& item : *userInfoVec) {
            item.second.at(nodeIndex).restoreDefaultValues();
        }

        // initialize empty feedback structures
        if (dir == UL || dir == DL) {
            for (auto&  hist : *history) {
                for (auto remote : remoteSet_) {
                    (hist.second)[remote].at(nodeIndex) = v;
                }
            }
        }
        else { // D2D
            for (auto& hit : *d2dHistory) {
                for (auto& ht : hit.second) {
                    if (ht.first == NODEID_NONE)                                          // skip fake UE 0
                        continue;

                    for (auto remote : remoteSet_) {
                        (ht.second)[remote].at(nodeIndex) = v;
                    }
                }
            }
        }
    }
    else {
        EV << "LteAmc::attachUser. Id " << nodeId << " is not known (it is the first time we see him)." << endl;

        // new user: [] operator insert a new element in the map
        (*nodeIndexMap)[nodeId] = (*revIndexVec).size();
        (*revIndexVec).push_back(nodeId);

        for (auto& item : *userInfoVec) {
            item.second.push_back(UserTxParams());
        }

        // get newly created index
        nodeIndex = (*nodeIndexMap).at(nodeId);

        // initialize empty feedback structures
        if (dir == UL || dir == DL) {
            for (auto& [key, hist] : *history) {
                for (auto remote : remoteSet_) {
                    hist[remote].push_back(v); // XXX DEBUG THIS!!
                }
            }
        }
        else { // D2D
            // initialize an empty feedback for a fake user (id 0), in order to manage
            // the case of transmission before a feedback has been reported
            for (auto& [key, hist] : *d2dHistory) {
                hist[NODEID_NONE] = History_();
                for (auto& [key2, d2dHistory] : hist) {
                    for (auto remote : remoteSet_) {
                        d2dHistory[remote].push_back(v); // XXX DEBUG THIS!!
                    }
                }
            }
        }
    }
    // Operation done in any case: use [] because new elements may be created
    (*connectedUe)[nodeId] = true;
}

void LteAmc::testUe(MacNodeId nodeId, Direction dir)
{
    EV << "##################################" << endl;
    EV << "LteAmc::testUe (" << dirToA(dir) << ")" << endl;

    ConnectedUesMap *connectedUe;
    std::map<MacNodeId, unsigned int> *nodeIndexMap;
    std::vector<MacNodeId> *revIndexVec;
    std::map<double, std::vector<UserTxParams>> *userInfoVec;
    std::map<double, History_> *history;
    std::map<double, std::map<MacNodeId, History_>> *d2dHistory;
    int numTxModes;

    if (dir == DL) {
        connectedUe = &dlConnectedUe_;
        nodeIndexMap = &dlNodeIndex_;
        revIndexVec = &dlRevNodeIndex_;
        userInfoVec = &dlTxParams_;
        history = &dlFeedbackHistory_;
        numTxModes = DL_NUM_TXMODE;
    }
    else if (dir == UL) {
        connectedUe = &ulConnectedUe_;
        nodeIndexMap = &ulNodeIndex_;
        revIndexVec = &ulRevNodeIndex_;
        userInfoVec = &ulTxParams_;
        history = &ulFeedbackHistory_;
        numTxModes = UL_NUM_TXMODE;
    }
    else if (dir == D2D) {
        connectedUe = &d2dConnectedUe_;
        nodeIndexMap = &d2dNodeIndex_;
        revIndexVec = &d2dRevNodeIndex_;
        userInfoVec = &d2dTxParams_;
        d2dHistory = &d2dFeedbackHistory_;
        numTxModes = UL_NUM_TXMODE;
    }
    else {
        throw cRuntimeError("LteAmc::attachUser(): Unrecognized direction");
    }

    unsigned int nodeIndex = (*nodeIndexMap).at(nodeId);
    bool isConnected = (*connectedUe).at(nodeId);
    MacNodeId revIndex = (*revIndexVec).at(nodeIndex);

    EV << "Id: " << nodeId << endl;
    EV << "Index: " << nodeIndex << endl;
    EV << "Reverse index: " << revIndex << " (should be the same as ID)" << endl;
    EV << "Is connected: " << (isConnected ? "TRUE" : "FALSE") << endl;

    if (!isConnected)
        return;

    // If connected compute and print user transmission parameters and history
    for (const auto& [key, value] : *userInfoVec) {
        UserTxParams info = value.at(nodeIndex);
        EV << "UserTxParams - carrier[" << key << "]" << endl;
        info.print("LteAmc::testUe");
    }

    if (dir == UL || dir == DL) {
        RemoteSet::iterator it = remoteSet_.begin();
        RemoteSet::iterator et = remoteSet_.end();
        std::vector<LteSummaryBuffer> feedback;

        for (const auto& hit : *history) {
            EV << "History" << endl;
            for ( ; it != et; it++ ) {
                EV << "Remote: " << dasToA(*it) << endl;
                feedback = (hit.second).at(*it).at(nodeIndex);
                for (int i = 0; i < numTxModes; i++) {
                    // Print only non-empty feedback summary! (all cqi are != NOSIGNALCQI)
                    Cqi testCqi = (feedback.at(i).get()).getCqi(Codeword(0), Band(0));
                    if (testCqi == NOSIGNALCQI)
                        continue;

                    feedback.at(i).get().print(NODEID_NONE, nodeId, dir, TxMode(i), "LteAmc::testUe");
                }
            }
        }
    }
    else { // D2D
        for (const auto& hit : *d2dHistory) {
            for (const auto& ht : hit.second) {
                const History_& d2dHistory = ht.second;
                std::vector<LteSummaryBuffer> feedback;

                EV << "History" << endl;
                for (auto remote : remoteSet_) {
                    EV << "Remote: " << dasToA(remote) << endl;
                    feedback = d2dHistory.at(remote).at(nodeIndex);
                    for (int i = 0; i < numTxModes; i++) {
                        // Print only non-empty feedback summary! (all cqi are != NOSIGNALCQI)
                        Cqi testCqi = (feedback.at(i).get()).getCqi(Codeword(0), Band(0));
                        if (testCqi == NOSIGNALCQI)
                            continue;

                        feedback.at(i).get().print(NODEID_NONE, nodeId, dir, TxMode(i), "LteAmc::testUe");
                    }
                }
            }
        }
    }
    EV << "##################################" << endl;
}

void LteAmc::setPilotMode(PilotComputationModes mode) { pilot_->setMode(mode); }

} //namespace

