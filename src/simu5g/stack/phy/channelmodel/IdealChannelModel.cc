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

#include "IdealChannelModel.h"

#include "simu5g/stack/phy/channelmodel/IRadioEndpoint.h"
#include "simu5g/stack/phy/radio/CellularReceiver.h"
#include "simu5g/stack/phy/radio/ICellularErrorModel.h"
#include "simu5g/stack/mac/amc/UserTxParams.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(IdealChannelModel);

// Reported on every band, in place of a computed value. Large enough that the AMC
// always picks the highest CQI, so the transport block size never varies with the
// channel and the only source of loss is the receiver's error model.
static const double FAKE_SINR_DB = 10000;

std::vector<double> IdealChannelModel::getSINR(IRadioEndpoint *radio, AirFrame *frame, UserControlInfo *lteInfo)
{
    std::vector<double> tmp(numBands_, FAKE_SINR_DB);
    // fake SINR is needed by the handover function to decide if the terminal should trigger the handover
    return tmp;
}

std::vector<double> IdealChannelModel::computeRsrp(IRadioEndpoint *receiver, const CellularTransmission& transmission)
{
    // fake RSRP is needed by the handover function to decide if the terminal should trigger the handover
    return std::vector<double>(numBands_, FAKE_SINR_DB);
}

std::vector<double> IdealChannelModel::computeSinr(IRadioEndpoint *receiver, const CellularTransmission& transmission)
{
    // fake SINR, as getSINR()
    return std::vector<double>(numBands_, FAKE_SINR_DB);
}

std::vector<double> IdealChannelModel::computeReceptionRsrp(IRadioEndpoint *receiver, AirFrame *frame)
{
    // fake RSRP, as computeRsrp()
    return std::vector<double>(numBands_, FAKE_SINR_DB);
}

bool IdealChannelModel::isReceptionSuccessful(IRadioEndpoint *radio, AirFrame *frame, UserControlInfo *lteInfo, const std::vector<double>& rsrpVector)
{
    // the CQI the codeword was sent with
    unsigned char cw = lteInfo->getCw();
    const auto& cqis = lteInfo->getUserTxParams()->readCqiVector();
    Cqi cqi = cqis[cqis.size() == 1 ? 0 : cw];
    if (cqi > 15)
        throw cRuntimeError("IdealChannelModel: a packet has been transmitted with a CQI greater than 15: %d", (int)cqi);
    if (lteInfo->getTxNumber() == 0)
        throw cRuntimeError("IdealChannelModel: the transmission counter should not be 0");

    // the receiving node's receiver decides, against its error model
    CellularReceiver *receiver = radio->getReceiver();
    ASSERT(receiver != nullptr); // no frame reaches a phantom radio
    std::vector<double> sinr = getSINR(radio, frame, lteInfo);
    std::optional<double> packetErrorRate = receiver->getErrorModel()->computePacketErrorRate((Direction)lteInfo->getDirection(), cqi, sinr,
            lteInfo->getGrantedBlocks(), lteInfo->getTxNumber());
    if (!packetErrorRate)
        return false; // lost for certain, no decision to draw
    bool success = receiver->decide(*packetErrorRate);
    EV << "IdealChannelModel::isReceptionSuccessful - transmission " << (int)lteInfo->getTxNumber()
       << ", error probability " << *packetErrorRate << " -> " << (success ? "received" : "lost") << endl;
    return success;
}

} //namespace
