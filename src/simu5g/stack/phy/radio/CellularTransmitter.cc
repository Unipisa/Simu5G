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

#include "simu5g/stack/phy/radio/CellularTransmitter.h"

#include "simu5g/common/LteControlInfo.h"

namespace simu5g {

Define_Module(CellularTransmitter);

CellularTransmission *CellularTransmitter::createTransmission(IRadioEndpoint *radio, const UserControlInfo& info, simtime_t duration) const
{
    auto transmission = new CellularTransmission();
    transmission->transmitter = radio;
    transmission->sourceId = info.getSourceId();
    transmission->destId = info.getDestId();
    transmission->direction = (Direction)info.getDirection();
    transmission->frameType = (LtePhyFrameType)info.getFrameType();
    transmission->carrierFrequency = info.getCarrierFrequency();
    transmission->grantedBlocks = info.getGrantedBlocks();
    transmission->txPower = info.getTxPower();
    transmission->startTime = simTime();
    transmission->duration = duration;
    return transmission;
}

} // namespace simu5g
