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

#include "simu5g/stack/phy/NtnPhyUe.h"

#include "simu5g/common/GeoUtils.h"
#include "simu5g/mobility/georeference/GeographicReferenceSystem.h"
#include "simu5g/stack/phy/feedback/LteDlFeedbackGenerator.h"
#include "simu5g/stack/phy/packet/NtnAirFrame.h"

namespace simu5g {

Define_Module(NtnPhyUe);

void NtnPhyUe::initialize(int stage)
{
    NrPhyUe::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        // Cached once here rather than looked up per transmission: the access class walks
        // the whole module tree from the system module on every call.
        referenceSystem_ = GeographicReferenceSystemAccess().get();
        if (referenceSystem_ == nullptr)
            throw cRuntimeError("NtnPhyUe::initialize - %s found no GeographicReferenceSystem module in the "
                    "network. A transparent NTN path cannot place its radios without one.", getFullPath().c_str());
        propagationDelay_.initialize(this, referenceSystem_);
    }
    else if (stage == INITSTAGE_SIMU5G_REGISTRATIONS2) {
        initializeChannelModels();
    }
}

void NtnPhyUe::initializeChannelModels()
{
    ntnAntennaModel_.reference(this, "ntnAntennaModelModule", true);

    primaryNtnChannelModel_.reference(this, "ntnChannelModelModule", true);
    primaryNtnChannelModel_->setPhy(this);
    GHz carrierFreq = primaryNtnChannelModel_->getCarrierFrequency();
    ntnChannelModel_[carrierFreq] = primaryNtnChannelModel_;

    int numChannelModels = primaryNtnChannelModel_->getVectorSize();
    for (int index = 1; index < numChannelModels; index++) {
        LteChannelModel *chanModel = check_and_cast<LteChannelModel *>(primaryNtnChannelModel_->getParentModule()->getSubmodule(primaryNtnChannelModel_->getName(), index));
        chanModel->setPhy(this);
        carrierFreq = chanModel->getCarrierFrequency();
        ntnChannelModel_[carrierFreq] = chanModel;
    }
}

void NtnPhyUe::sendUnicast(LteAirFrame *airFrame)
{
    if (sendUnicastViaNtn(airFrame))
        return;

    NrPhyUe::sendUnicast(airFrame);
}

bool NtnPhyUe::shouldSendViaTransparentNtn(MacNodeId destId) const
{
    if (servingNodeId_ == NODEID_NONE || destId != servingNodeId_)
        return false;

    const GnbNtnAssociation *association = binder_->getGnbNtnAssociation(servingNodeId_);
    return association != nullptr && association->isTransparent;
}

bool NtnPhyUe::sendUnicastViaNtn(LteAirFrame *airFrame)
{
    auto *ci = check_and_cast<UserControlInfo *>(airFrame->getControlInfo());
    MacNodeId destId = ci->getDestId();
    if (!shouldSendViaTransparentNtn(destId))
        return false;

    auto *ntnAirFrame = dynamic_cast<NtnAirFrame *>(airFrame);
    if (ntnAirFrame == nullptr) {
        ntnAirFrame = new NtnAirFrame(airFrame->getName());
        ntnAirFrame->setKind(airFrame->getKind());
        ntnAirFrame->setDuration(airFrame->getDuration());
        ntnAirFrame->setSchedulingPriority(airFrame->getSchedulingPriority());
        if (airFrame->getControlInfo() != nullptr)
            ntnAirFrame->setControlInfo(airFrame->removeControlInfo());
        ntnAirFrame->encapsulate(airFrame->decapsulate());
        delete airFrame;
        airFrame = ntnAirFrame;
    }

    const GnbNtnAssociation *association = binder_->getGnbNtnAssociation(servingNodeId_);
    SatelliteInfo *satelliteInfo = binder_->getSatelliteInfo(association->satelliteId);
    if (satelliteInfo == nullptr || satelliteInfo->satelliteModule == nullptr)
        throw cRuntimeError("NtnPhyUe::sendUnicastViaNtn - satellite %hu for serving node %hu is not registered", num(association->satelliteId), num(servingNodeId_));

    cGate *serviceLinkGate = satelliteInfo->satelliteModule->gate("serviceLinkRadioIn");
    if (serviceLinkGate == nullptr)
        throw cRuntimeError("NtnPhyUe::sendUnicastViaNtn - satellite %s has no serviceLinkRadioIn gate", satelliteInfo->satelliteModule->getFullPath().c_str());

    if (airFrame->getControlInfo() != nullptr) {
        UserControlInfo *userControlInfo = check_and_cast<UserControlInfo *>(airFrame->removeControlInfo());
        userControlInfo->setRadioTransmitterId(nodeId_);
        userControlInfo->setRadioTransmitterCoord(getRadioPosition());
        userControlInfo->setRadioTransmitterEcefCoord(propagationDelay_.ecefFromRadioPosition(getRadioPosition()));
        userControlInfo->setRadioTransmitterAntenna(ntnAntennaModel_);
        userControlInfo->setRadioReceiverId(association->satelliteId);
        airFrame->setAdditionalInfo(*userControlInfo);
        delete userControlInfo;
    }

    // Computed outside the block above: the hop takes just as long whether or not the frame
    // happens to carry control info.
    simtime_t delay = propagationDelay_.computeHopDelay(getRadioPosition(), serviceLinkGate,
            nodeId_, association->satelliteId);
    EV << NOW << " NtnPhyUe::sendUnicastViaNtn - forwarding frame for serving node "
       << destId << " to satellite " << association->satelliteId
       << ", propagationDelay[" << delay << "]" << endl;
    sendDirect(airFrame, delay, propagationDelay_.transmissionDuration(nodeType_, airFrame->getDuration()),
            serviceLinkGate);
    return true;
}

LteChannelModel *NtnPhyUe::getNtnChannelModel(GHz carrierFreq) const
{
    auto it = ntnChannelModel_.find(carrierFreq);
    return (it == ntnChannelModel_.end()) ? nullptr : it->second;
}

LteChannelModel *NtnPhyUe::getReceptionChannelModel(const UserControlInfo *lteInfo)
{
    GHz carrierFreq = lteInfo->getCarrierFrequency();

    // Transparent NTN relaying currently preserves the serving gNB as sourceId.
    // Because of that, we infer "satellite-originated" reception from the source
    // gNB's NTN association instead of checking for a SATELLITE_NODE source here.
    // Future review note: if frame sourceId semantics change to expose the actual
    // satellite ID on downlink frames, this lookup logic should be updated.
    const GnbNtnAssociation *association = binder_->getGnbNtnAssociation(lteInfo->getSourceId());
    if (association == nullptr || !association->isTransparent)
        return NrPhyUe::getReceptionChannelModel(lteInfo);

    EV_DEBUG << "NtnPhyUe::getReceptionChannelModel - using NTN channel model" << endl;
    auto it = ntnChannelModel_.find(carrierFreq);
    return (it == ntnChannelModel_.end()) ? nullptr : it->second;
}

double NtnPhyUe::computeReceivedBeaconPacketRssi(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    const GnbNtnAssociation *association = binder_->getGnbNtnAssociation(lteInfo->getSourceId());
    if (association == nullptr || !association->isTransparent)
        return NrPhyUe::computeReceivedBeaconPacketRssi(frame, lteInfo);

    LteChannelModel *channelModel = getReceptionChannelModel(lteInfo);
    if (channelModel == nullptr)
        throw cRuntimeError("NtnPhyUe::computeReceivedBeaconPacketRssi - no NTN channel model for carrier %gGHz", lteInfo->getCarrierFrequency().get());

    std::vector<double> rsrpV = channelModel->computeReceptionRsrp(frame, lteInfo);
    double rsrp = 0;
    for (double value : rsrpV)
        rsrp += value;
    return rsrp / rsrpV.size();
}

} //namespace
