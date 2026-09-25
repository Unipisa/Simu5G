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

#include "simu5g/stack/phy/medium/CellularRadioMedium.h"

#include <inet/common/INETMath.h>

#include "simu5g/stack/phy/channelmodel/IRadioEndpoint.h"
#include "simu5g/stack/phy/packet/AirFrame_m.h"

namespace simu5g {

Define_Module(CellularRadioMedium);

void CellularRadioMedium::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        // the distance at which a free-space transmission at pMax falls to the sat threshold
        double waveLength = SPEED_OF_LIGHT / par("carrierFrequency").doubleValue();
        double minReceivePower = pow(10.0, par("sat").doubleValue() / 10.0);
        maxInterferenceDistance = pow(waveLength * waveLength * par("pMax").doubleValue()
                / (16.0 * M_PI * M_PI * minReceivePower), 1.0 / par("alpha").doubleValue());
        EV << "max interference distance:" << maxInterferenceDistance << endl;
    }
}

void CellularRadioMedium::addRadio(MacNodeId nodeId, IRadioEndpoint *radio, cModule *radioModule)
{
    Enter_Method("addRadio");
    if (nodeId != NODEID_NONE && !radios.emplace(nodeId, radio).second)
        throw cRuntimeError("CellularRadioMedium::addRadio(): a radio is already registered for node %d", (int)num(nodeId));
    if (radioModule != nullptr)
        radioModules[check_and_cast<cModule *>(radio)->getId()] = RadioModule{radio, radioModule->gate("radioIn")->getPathStartGate()};
}

void CellularRadioMedium::removeRadio(IRadioEndpoint *radio)
{
    Enter_Method("removeRadio");
    bool found = false;
    for (auto it = radios.begin(); it != radios.end(); ++it) {
        if (it->second == radio) {
            radios.erase(it);
            found = true;
            break;
        }
    }
    if (auto module = dynamic_cast<cModule *>(radio))
        found = radioModules.erase(module->getId()) != 0 || found;
    if (!found)
        throw cRuntimeError("CellularRadioMedium::removeRadio(): the radio is not on the medium");
}

IRadioEndpoint *CellularRadioMedium::findRadio(MacNodeId nodeId) const
{
    auto it = radios.find(nodeId);
    return it == radios.end() ? nullptr : it->second;
}

void CellularRadioMedium::sendToNeighbors(IRadioEndpoint *sender, cSimpleModule *sendingModule, AirFrame *frame, simtime_t duration)
{
    // NOTE: no Enter_Method(): the copies are sent by the sending radio module
    const inet::Coord& senderPosition = sender->getCoord();
    double maxDistanceSquared = maxInterferenceDistance * maxInterferenceDistance;
    for (const auto& [moduleId, radioModule] : radioModules) {
        if (radioModule.radio == sender)
            continue;
        if (senderPosition.sqrdist(radioModule.radio->getCoord()) < maxDistanceSquared) {
            EV << "sending message to radio\n";
            // no propagation delay, as for the frames the PHY sends to a single radio
            sendingModule->sendDirect(frame->dup(), 0, duration, radioModule.radioInGate);
        }
    }
    // the radios in range got copies; the original frame can be deleted
    delete frame;
}

IRadioEndpoint *CellularRadioMedium::getRadio(MacNodeId nodeId) const
{
    IRadioEndpoint *radio = findRadio(nodeId);
    if (radio == nullptr)
        throw cRuntimeError("CellularRadioMedium::getRadio(): no radio is registered for node %d", (int)num(nodeId));
    return radio;
}

} // namespace simu5g
