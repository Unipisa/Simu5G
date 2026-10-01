//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/d2d/phy/channelmodel/D2dChannelModel.h"

#include "simu5g/background/trafficGenerator/generators/TrafficGeneratorBase.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/stack/d2d/mac/ID2dMacEnb.h"
#include "simu5g/stack/mac/amc/UserTxParams.h"
#include "simu5g/stack/phy/PhyUe.h"
#include "simu5g/stack/phy/packet/AirFrame_m.h"

namespace simu5g {

Define_Module(D2dChannelModel);

void D2dChannelModel::initialize(int stage)
{
    StochasticChannelModel::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        rcvdSinrD2DSignal_ = cComponent::registerSignal("rcvdSinrD2D");
        enableD2DInterference_ = par("d2dInterference");
    }
}

RadioLink D2dChannelModel::d2dLink(MacNodeId srcId, Coord srcCoord, MacNodeId destId, Coord destCoord)
{
    RadioLink link;
    link.dir = D2D;

    link.txId = srcId;
    link.rxId = destId;
    link.txCoord = srcCoord;
    link.rxCoord = destCoord;

    // Both endpoints are UEs.
    link.txRadio = radioMedium_->findRadio(srcId);
    link.rxRadio = radioMedium_->findRadio(destId);
    link.txIsBaseStation = false; // omnidirectional: no angular attenuation

    // The channel state is keyed on the *link*, so a UE's several D2D peers each
    // get their own LOS state, shadowing realization and fading process.
    //
    // The node whose position history defines the speed is the transmitter.
    link.linkKey = LinkKey(link.txId, link.rxId);
    link.stateNodeId = srcId;
    link.stateCoord = srcCoord;

    return link;
}

std::vector<double> D2dChannelModel::getRSRP_D2D(AirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, Coord destCoord)
{
    EV << "------------ GET RSRP D2D----------------" << endl;

    RadioLink link = d2dLink(lteInfo_1->getSourceId(), lteInfo_1->getCoord(), destId, destCoord);

    // Note the D2D-specific transmit power: a D2D transmission does not use the
    // power the UE would use towards the base station.
    return getRSRP(link, lteInfo_1->getD2dTxPower());
}

std::vector<double> D2dChannelModel::getSINR_D2D(AirFrame *frame, UserControlInfo *lteInfo, MacNodeId destId, Coord destCoord, MacNodeId enbId)
{
    // desired-signal RSRP (pathloss + shadowing + fading), then noise and
    // interference on top: exactly the two halves this body used to inline
    std::vector<double> rsrpVector = getRSRP_D2D(frame, lteInfo, destId, destCoord);
    return getSINR_D2D(frame, lteInfo, destId, destCoord, enbId, rsrpVector);
}

std::vector<double> D2dChannelModel::getSINR_D2D(AirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, Coord destCoord, MacNodeId enbId, const std::vector<double>& rsrpVector)
{
    EV << "------------ GET SINR D2D----------------" << endl;

    // The desired signal is already known; the core adds noise and interference,
    // asking computeInterferencePlusNoise() below for the D2D denominator.
    RadioLink link = d2dLink(lteInfo_1->getSourceId(), lteInfo_1->getCoord(), destId, destCoord);
    link.cellId = enbId;

    // The caller is expected to supply one RSRP value per band. The one-to-many
    // capture-effect path only fills bestRsrpVector_ when the capture factor is
    // "RSRP"; with "Distance" it stays empty, and indexing it below would be out
    // of bounds. Fall back to computing the desired signal here rather than
    // reading past the end.
    if (rsrpVector.size() < numBands_) {
        if (!rsrpVector.empty())
            throw cRuntimeError("D2dChannelModel::getSINR_D2D - RSRP vector has %zu entries, expected %u",
                    rsrpVector.size(), numBands_);
        return getSINR(link, lteInfo_1, getRSRP(link, lteInfo_1->getD2dTxPower()));
    }

    return getSINR(link, lteInfo_1, rsrpVector);
}

void D2dChannelModel::computeInterferencePlusNoise(const RadioLink& link, UserControlInfo *lteInfo,
        RbMap& rbmap, double totN, std::vector<double>& den)
{
    if (link.dir != D2D && link.dir != D2D_MULTI) {
        StochasticChannelModel::computeInterferencePlusNoise(link, lteInfo, rbmap, totN, den);
        return;
    }

    /*
     * In calculating a D2D CQI, the interference from other D2D UEs discriminates between calculating a CQI
     * in the direction D2D_Tx--->D2D_Rx or D2D_Tx<---D2D_Rx (This happens due to the different positions of the
     * interfering UEs relative to the position of the UE for whom we are calculating the CQI). We need that the CQI
     * for the D2D_Tx is the same as the D2D_Rx (This is a help for the simulator because when the eNodeB allocates
     * resources to a D2D_Tx it must refer to the quality channel of the D2D_Rx).
     * To do so, here we must check if the ueId is the ID of the D2D_Tx: if it
     * is so we swap the ueId with the one of his Peer (D2D_Rx). We do the same for the coord.
     */
    // vector containing the sum of inCell interference for each band
    std::vector<double> d2dInterference; // Linear value (mW)
    d2dInterference.resize(numBands_, 0);
    if (enableD2DInterference_) {
        computeD2DInterference(link.cellId, link.txId, link.txCoord, link.rxId, link.rxCoord,
                (lteInfo->getFrameType() == FEEDBACKPKT), lteInfo->getCarrierFrequency(), rbmap, &d2dInterference, link.dir);
    }

    EV << "D2dChannelModel::computeInterferencePlusNoise - distance from my Peer = "
       << link.rxCoord.distance(link.txCoord) << " - DIR=" << dirToA(link.dir) << endl;

    // One loop for both cases: with interference disabled d2dInterference is all
    // zeros, so this degenerates to the noise-only denominator. (The two used to be
    // written out separately, the disabled branch summing noise directly instead of
    // going through dBm -> linear -> dBm. That round trip is not bit-identical, so
    // the collapse is observable -- but only where d2dInterference is false, which
    // no shipped configuration sets.)
    for (unsigned int i = 0; i < numBands_; i++) {
        // the caller skips these bands too; leave their denominator untouched
        if (lteInfo->getFrameType() == DATAPKT && rbmap[MACRO][i] == 0)
            continue;

        den[i] = linearToDBm(totN + d2dInterference[i]);

        EV << "\t in[" << d2dInterference[i] << "] - den[" << den[i] << "]\n";
    }
}

std::vector<double> D2dChannelModel::getReceptionSinr(AirFrame *frame, UserControlInfo *lteInfo, const std::vector<double>& rsrpVector)
{
    Direction dir = lteInfo->getDirection();
    if (dir == D2D || dir == D2D_MULTI) {
        MacNodeId destId = lteInfo->getDestId();
        Coord destCoord = phy_->getCoord();
        MacNodeId enbId = binder_->getServingNodeOrSelf(lteInfo->getSourceId());

        // One-to-many reception decides on the RSRP captured by the capture-effect
        // logic (see D2dUePhyHelper::storeAirFrame), so the desired signal is not
        // recomputed here.
        if (dir == D2D_MULTI)
            return getSINR_D2D(frame, lteInfo, destId, destCoord, enbId, rsrpVector);
        return getSINR_D2D(frame, lteInfo, destId, destCoord, enbId);
    }
    return StochasticChannelModel::getReceptionSinr(frame, lteInfo, rsrpVector);
}

void D2dChannelModel::emitRcvdSinr(Direction dir, MacNodeId ueId, GHz carrierFrequency, double sinr)
{
    if (dir == D2D || dir == D2D_MULTI) {
        // A D2D reception is not an uplink reception. Attribute it to the receiver --
        // this module -- which is also what the one-to-many path already does, rather
        // than to the sender's channel model the way the core's UL branch does.
        emit(rcvdSinrD2DSignal_, sinr);
        return;
    }
    StochasticChannelModel::emitRcvdSinr(dir, ueId, carrierFrequency, sinr);
}

bool D2dChannelModel::computeD2DInterference(MacNodeId eNbId, MacNodeId senderId, Coord senderCoord, MacNodeId destId, Coord destCoord, bool isCqi, GHz carrierFrequency,
        const RbMap& rbmap, std::vector<double> *interference, Direction dir)
{
    EV << "**** D2D Interference for cellId[" << eNbId << "] node[" << destId << "] ****" << endl;

    // get the D2D view of the eNodeB's MAC
    ID2dMacEnb *macEnb = check_and_cast<ID2dMacEnb *>(binder_->getMacFromMacNodeId(eNbId));

    // what an interfering UE adds on the band
    auto addInterference = [&] (const InterfererInfo& interferer, unsigned int band) {
        const MacNodeId ueId = interferer.nodeId;
        const MacCellId cellId = interferer.cellId;
        const Direction dir = interferer.dir;

        // no self-interference
        if (ueId == senderId || ueId == destId)
            return;

        // no interference from UL connections of the same cell (no D2D-UL reuse allowed)
        if (dir == UL && cellId == eNbId)
            return;

        // no interference from D2D connections of the same cell when reuse is disabled (otherwise, computation of CQI is misleading)
        if (cellId == eNbId && (!macEnb->isReuseD2DEnabled() && !macEnb->isReuseD2DMultiEnabled()))
            return;

        EV << NOW << " D2dChannelModel::computeD2DInterference - Interference from UE: " << ueId << "(dir " << dirToA(dir) << ") on band[" << band << "]" << endl;

        // get tx power and attenuation from this UE
        double rxPwr = interferer.txPwr - cableLossOf(destId) + antennaGainOf(ueId, antennaGainUe_) + antennaGainOf(destId, antennaGainUe_);
        // interferer -> our receiver
        double att = getAttenuation(d2dLink(ueId, interferer.coord, destId, destCoord));
        (*interference)[band] += dBmToLinear(rxPwr - att);//(dBm-dB)=dBm

        EV << "\t band " << band << "/pwr[" << rxPwr - att << "]-int[" << (*interference)[band] << "]" << endl;
    };

    if (isCqi) {
        // CQI computation checks the slot occupation of the current TTI
        const std::vector<std::vector<UeAllocationInfo>> *ulTransmissionMap = binder_->getUlTransmissionMap(carrierFrequency, CURR_TTI);
        if (ulTransmissionMap != nullptr && !ulTransmissionMap->empty()) {
            for (unsigned int i = 0; i < numBands_; i++) {
                // the UEs transmitting on the same band
                for (auto& allocation : ulTransmissionMap->at(i))
                    addInterference(StochasticChannelModel::describeInterferer(allocation), i);
            }
        }
    }
    else {
        // error computation: the uplink transmissions of the slot just completed;
        // the Binder's map of the previous TTI lists the same ones, band by band in the same order
        ASSERT(radioMedium_->matchesUplinkTransmissionMap(carrierFrequency, binder_->getUlTransmissionMap(carrierFrequency, PREV_TTI)));
        const auto ulTransmissions = radioMedium_->getUplinkTransmissionsByBand(carrierFrequency);
        for (unsigned int i = 0; i < numBands_ && i < ulTransmissions.size(); i++) {
            // if we are decoding a data transmission and this RB has not been used, skip it
            // TODO fix for multi-antenna case
            if (!rbmap.empty() && rbmap.at(MACRO).at(i) == 0)
                continue;

            // the transmissions on the same band, in the order they were created
            for (auto transmission : ulTransmissions[i])
                addInterference(StochasticChannelModel::describeInterferer(*transmission), i);
        }
    }

    // Debug Output
    EV << NOW << " D2dChannelModel::computeD2DInterference - Final Band Interference Status: " << endl;
    for (unsigned int i = 0; i < numBands_; i++)
        EV << "\t band " << i << " int[" << (*interference)[i] << "]" << endl;

    return true;
}

} //namespace
