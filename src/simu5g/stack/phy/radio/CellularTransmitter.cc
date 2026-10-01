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
#include "simu5g/stack/phy/channelmodel/IRadioEndpoint.h"
#include "simu5g/stack/phy/radio/RadioTransmissionRequest.h"

namespace simu5g {

Define_Module(CellularTransmitter);

CellularTransmission *CellularTransmitter::createTransmission(const RadioTransmissionRequest& request, const UserControlInfo& info) const
{
    auto transmission = new CellularTransmission();
    transmission->transmitter = request.sender;
    transmission->sourceId = info.getSourceId();
    transmission->destId = info.getDestId();
    transmission->cellId = request.cellId;
    transmission->direction = (Direction)info.getDirection();
    transmission->frameType = (LtePhyFrameType)info.getFrameType();
    transmission->carrierFrequency = info.getCarrierFrequency();
    transmission->grantedBlocks = info.getGrantedBlocks();
    // a one-to-one D2D frame is sent at the UE's D2D power, which its control info carries besides the cellular one.
    // NOTE: a one-to-many D2D frame is recorded at the cellular power, the power other receivers' interference
    // takes it at, although its own receivers take the D2D power
    transmission->txPower = transmission->direction == D2D ? info.getD2dTxPower() : info.getTxPower();
    transmission->startPosition = request.sender->getCoord();
    transmission->startTime = simTime();
    transmission->duration = request.duration;
    return transmission;
}

} // namespace simu5g
