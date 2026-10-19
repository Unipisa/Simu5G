//
//                  Simu5G
//
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/phy/radio/CellularPhantomRadio.h"

#include "simu5g/stack/phy/radio/CellularAntenna.h"
#include "simu5g/stack/phy/radio/CellularTransmitter.h"

namespace simu5g {

Define_Module(CellularPhantomRadio);

void CellularPhantomRadio::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        mobility_.reference(this, "mobilityModule", false);
        antenna_ = check_and_cast<CellularAntenna *>(getSubmodule("antenna"));
        isNr_ = par("isNr");
        txPower_ = par("txPower");
        noiseFigure_ = par("noiseFigure");
        cableLoss_ = par("cableLoss");
        WATCH(nodeId_);
    }
}

CellularPhantomRadio::~CellularPhantomRadio()
{
    // nodeId_ is set once the radio is on the medium; radioMedium_ is unset if the medium has already been deleted
    if (nodeId_ != NODEID_NONE && radioMedium_)
        radioMedium_->removeRadio(this);
}

void CellularPhantomRadio::addToMedium(MacNodeId nodeId)
{
    Enter_Method("addToMedium");
    if (nodeId_ != NODEID_NONE)
        throw cRuntimeError("CellularPhantomRadio::addToMedium(): the radio is already on the medium as node %d", (int)num(nodeId_));
    // resolved here, not at initialization: the owner may add the radio before it has initialized
    radioMedium_.reference(this, "radioMediumModule", true);
    // not a radio frames are sent to: no radio module
    radioMedium_->addRadio(nodeId, this);
    nodeId_ = nodeId;
}

void CellularPhantomRadio::transmit(MacNodeId destId, MacCellId cellId, Direction direction, GHz carrierFrequency, const RbMap& blocks, simtime_t duration,
        const cModule *phantomCell)
{
    Enter_Method("transmit");
    if (nodeId_ == NODEID_NONE)
        throw cRuntimeError("CellularPhantomRadio::transmit(): the radio is not on the medium");
    auto transmitter = check_and_cast<CellularTransmitter *>(getSubmodule("transmitter"));
    radioMedium_->addPhantomTransmission(transmitter->createPhantomTransmission(this, destId, cellId, direction, carrierFrequency, blocks, duration, phantomCell));
}

const inet::Coord& CellularPhantomRadio::getCoord() const
{
    if (mobility_ != nullptr)
        position_ = mobility_->getCurrentPosition();
    return position_;
}

double CellularPhantomRadio::getSpeed()
{
    return mobility_ != nullptr ? mobility_->getCurrentVelocity().length() : 0.0;
}

TxDirectionType CellularPhantomRadio::getTxDirection()
{
    return antenna_->getTxDirection();
}

double CellularPhantomRadio::getTxAngle()
{
    return antenna_->getTxAngle();
}

double CellularPhantomRadio::getAntennaGain()
{
    return antenna_->getGain();
}

ChannelModelBase *CellularPhantomRadio::getChannelModel(GHz carrierFreq)
{
    if (carrierFreq == GHz(0.0))
        return nullptr;
    return radioMedium_->getChannelModel(carrierFreq, isNr_);
}

} // namespace simu5g
