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

#include <algorithm>

#include <inet/common/INETMath.h>

#include "simu5g/background/trafficGenerator/generators/TrafficGeneratorBase.h"
#include "simu5g/stack/phy/channelmodel/IRadioEndpoint.h"
#include "simu5g/stack/phy/packet/AirFrame_m.h"
#include "simu5g/stack/phy/PhyBase.h"
#include "simu5g/stack/phy/channelmodel/ChannelModelBase.h"

namespace simu5g {

Define_Module(CellularRadioMedium);

ChannelModelBase *CellularRadioMedium::getChannelModel(GHz carrierFrequency, bool isNr) const
{
    const char *name = isNr ? "nrChannelModel" : "channelModel";
    for (int i = 0; i < par("numCarriers").intValue(); i++) {
        auto *model = check_and_cast<ChannelModelBase *>(getSubmodule(name, i));
        if (model->getCarrierFrequency() == carrierFrequency)
            return model;
    }
    return nullptr;
}

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

CellularRadioMedium::~CellularRadioMedium()
{
    for (auto& [carrier, carrierTransmissions] : transmissions)
        for (auto transmission : carrierTransmissions)
            delete transmission;
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

void CellularRadioMedium::addTransmission(CellularTransmission *transmission)
{
    Enter_Method("addTransmission");
    transmission->id = nextTransmissionId++;
    auto& carrierTransmissions = transmissions[transmission->carrierFrequency];
    // the slot before this one is kept, for the questions about the slot completed last
    simtime_t keptSince = transmission->startTime - transmission->duration;
    auto ended = std::remove_if(carrierTransmissions.begin(), carrierTransmissions.end(), [keptSince] (const CellularTransmission *t) {
        if (t->getEndTime() < keptSince) {
            delete t;
            return true;
        }
        return false;
    });
    carrierTransmissions.erase(ended, carrierTransmissions.end());
    carrierTransmissions.push_back(transmission);
}

std::vector<const CellularTransmission *> CellularRadioMedium::getDataTransmissionsDuring(GHz carrierFrequency, simtime_t from, simtime_t to) const
{
    std::vector<const CellularTransmission *> during;
    auto it = transmissions.find(carrierFrequency);
    if (it == transmissions.end())
        return during;
    for (auto t : it->second)
        if (t->frameType == DATAPKT && t->startTime < to && t->getEndTime() > from)
            during.push_back(t);
    return during;
}

std::vector<const CellularTransmission *> CellularRadioMedium::getUplinkTransmissionsDuring(GHz carrierFrequency, simtime_t from, simtime_t to) const
{
    std::vector<const CellularTransmission *> uplink;
    for (auto t : getDataTransmissionsDuring(carrierFrequency, from, to))
        if (t->direction == UL || t->direction == D2D || t->direction == D2D_MULTI)
            uplink.push_back(t);
    return uplink;
}

bool CellularRadioMedium::matchesUplinkTransmissionMap(GHz carrierFrequency, simtime_t from, simtime_t to, const std::vector<std::vector<UeAllocationInfo>> *mapOrNull) const
{
    static const std::vector<std::vector<UeAllocationInfo>> noMap;
    const auto& map = mapOrNull != nullptr ? *mapOrNull : noMap;
    // the registry's view, band by band
    std::vector<std::vector<const CellularTransmission *>> view(map.size());
    for (auto t : getUplinkTransmissionsDuring(carrierFrequency, from, to)) {
        // background cells' UEs are not in the Binder's map
        if (t->phantomCell != nullptr)
            continue;
        auto antennaIt = t->grantedBlocks.find(MACRO);
        if (antennaIt == t->grantedBlocks.end())
            continue;
        for (const auto& [band, allocation] : antennaIt->second) {
            if (allocation == 0)
                continue;
            if (band >= view.size())
                return false;
            view[band].push_back(t);
        }
    }
    for (size_t band = 0; band < map.size(); band++) {
        size_t i = 0;
        for (const auto& info : map[band]) {
            if (i >= view[band].size())
                return false;
            auto t = view[band][i++];
            // a PHY's entry is its radio's frame, a background UE's entry its registered allocation
            if (info.nodeId != t->sourceId || info.dir != t->direction
                    || static_cast<IRadioEndpoint *>(info.phy) != t->transmitter || info.trafficGen != t->backgroundUe)
                return false;
            // the cell and the power the interference computation takes for the entry
            double txPower = info.phy != nullptr ? info.phy->getTxPwr(info.dir) : info.trafficGen->getTxPwr();
            if (info.cellId != t->cellId || txPower != t->txPower)
                return false;
        }
        if (i != view[band].size())
            return false;
    }
    return true;
}

bool CellularRadioMedium::isBandOccupied(GHz carrierFrequency, MacNodeId sourceId, Direction direction, Band band, simtime_t from, simtime_t to) const
{
    for (auto t : getDataTransmissionsDuring(carrierFrequency, from, to)) {
        if (t->sourceId != sourceId || t->direction != direction)
            continue;
        auto antennaIt = t->grantedBlocks.find(MACRO);
        if (antennaIt == t->grantedBlocks.end())
            continue;
        auto bandIt = antennaIt->second.find(band);
        if (bandIt != antennaIt->second.end() && bandIt->second != 0)
            return true;
    }
    return false;
}

IRadioEndpoint *CellularRadioMedium::getRadio(MacNodeId nodeId) const
{
    IRadioEndpoint *radio = findRadio(nodeId);
    if (radio == nullptr)
        throw cRuntimeError("CellularRadioMedium::getRadio(): no radio is registered for node %d", (int)num(nodeId));
    return radio;
}

ChannelState& CellularRadioMedium::getChannelState(const cComponent *channelModel)
{
    // NOTE: no Enter_Method(): data access only, as for findRadio()
    return channelStates[channelModel];
}

void CellularRadioMedium::removeChannelState(const cComponent *channelModel)
{
    channelStates.erase(channelModel);
}

ChannelState::LosMap& CellularRadioMedium::getLosMap(GHz carrierFrequency)
{
    return losMaps[carrierFrequency];
}

ChannelState::ShadowFadingMap& CellularRadioMedium::getShadowingMap(GHz carrierFrequency)
{
    return shadowingMaps[carrierFrequency];
}

ChannelState::JakesFadingMap& CellularRadioMedium::getJakesFadingMap(GHz carrierFrequency)
{
    return jakesFadingMaps[carrierFrequency];
}

} // namespace simu5g
