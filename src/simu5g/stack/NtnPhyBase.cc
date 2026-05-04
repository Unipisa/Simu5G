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

#include "simu5g/stack/NtnPhyBase.h"

#include <inet/common/ModuleAccess.h>

#include "simu5g/common/GeoUtils.h"
#include "simu5g/stack/phy/packet/LteAirFrame_m.h"
#include "simu5g/stack/phy/packet/NtnAirFrame.h"

#include <cmath>

namespace simu5g {

using namespace omnetpp;
using namespace inet;

Define_Module(NtnPhyBase);

void NtnPhyBase::initialize(int stage)
{
    ChannelAccess::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        binder_.reference(this, "binderModule", true);
        antennaModel_.reference(this, "antennaModelModule", true);
        referenceSystem_ = GeographicReferenceSystemAccess().get();
        ASSERT(referenceSystem_ != nullptr);
        isFeederLink_ = par("linkType").stdstringValue() == "feeder";
        feederLinkFrequencyOffset_ = GHz(par("feederLinkFrequencyOffset"));
        cModule *node = getContainingNode(this);
        nodeId_ = MacNodeId(node->par("macNodeId").intValue());
        nodeType_ = aToNodeType(node->par("nodeType").stdstringValue());
    }
    else if (stage == INITSTAGE_SIMU5G_REGISTRATIONS2) {
        initializeChannelModels();
    }
}

void NtnPhyBase::initializeChannelModels()
{
    primaryChannelModel_.reference(this, "channelModelModule", true);
    primaryChannelModel_->setPhy(this);
    GHz carrierFreq = primaryChannelModel_->getCarrierFrequency();
    channelModel_[isFeederLink_ ? GHz(carrierFreq.get() + feederLinkFrequencyOffset_.get()) : carrierFreq] = primaryChannelModel_;

    int numChannelModels = primaryChannelModel_->getVectorSize();
    for (int index = 1; index < numChannelModels; index++) {
        LteChannelModel *chanModel = check_and_cast<LteChannelModel *>(primaryChannelModel_->getParentModule()->getSubmodule(primaryChannelModel_->getName(), index));
        chanModel->setPhy(this);
        carrierFreq = chanModel->getCarrierFrequency();
        if (isFeederLink_)
            carrierFreq = shiftFrequencyBand(carrierFreq);
        channelModel_[carrierFreq] = chanModel;
    }
}

LteChannelModel *NtnPhyBase::getChannelModel(GHz carrierFreq) const
{
    auto it = channelModel_.find(carrierFreq);
    return (it == channelModel_.end()) ? nullptr : it->second;
}

GHz NtnPhyBase::shiftFrequencyBand(GHz carrierFreq) const
{
    if (std::isnan(carrierFreq.get()))
        return carrierFreq;

    double transmitFrequency = isFeederLink_
        ? carrierFreq.get() + feederLinkFrequencyOffset_.get()
        : carrierFreq.get() - feederLinkFrequencyOffset_.get();

    if (transmitFrequency <= 0.0)
        throw cRuntimeError("NtnPhyBase::shiftFrequencyBand - cannot translate %s-link carrier %gGHz with offset %gGHz",
                isFeederLink_ ? "feeder" : "service", carrierFreq.get(), feederLinkFrequencyOffset_.get());
    return GHz(transmitFrequency);
}

void NtnPhyBase::handleAirFrame(cMessage *msg)
{
    auto *frame = check_and_cast<LteAirFrame *>(msg);
    UserControlInfo lteInfo(frame->getAdditionalInfo());
    EV << "NtnPhyBase::handleAirFrame - received air frame " << msg->getName() << " from " << (isFeederLink_ ? "feeder" : "service") << " link radio"
            << ", carrierFreq[" << lteInfo.getCarrierFrequency() << "]" << endl;

    if (lteInfo.getFrameType() == DATAPKT) {
        GHz carrierFrequency = lteInfo.getCarrierFrequency();
        LteChannelModel *channelModel = getChannelModel(carrierFrequency);
        if (channelModel != nullptr) {
            if (nodeType_ == SATELLITE_NODE) {
                auto *ntnFrame = dynamic_cast<NtnAirFrame *>(frame);
                if (ntnFrame == nullptr)
                    throw cRuntimeError("NtnPhyBase::handleAirFrame - transparent NTN data frame %s is not an NtnAirFrame", frame->getFullName());
                std::vector<double> sinrVector = channelModel->getSINR(frame, &lteInfo);
                ntnFrame->setRelayHopSinrVector(sinrVector);
                EV << "NtnPhyBase::handleAirFrame - forward the frame to the " << (isFeederLink_ ? "service" : "feeder") << " NIC for relaying" << endl;
            }
            else {
                auto *ntnFrame = dynamic_cast<NtnAirFrame *>(frame);
                if (ntnFrame == nullptr)
                    throw cRuntimeError("NtnPhyBase::handleAirFrame - transparent NTN data frame %s is not an NtnAirFrame", frame->getFullName());

                bool result = channelModel->isReceptionSuccessful(frame, &lteInfo);
                ntnFrame->setGatewayReceptionResultInfo(result);
                EV << "NtnPhyBase::handleAirFrame - handled LteAirframe with ID " << frame->getId() << " with result " << (result ? "RECEIVED" : "NOT RECEIVED") << endl;
                EV << "NtnPhyBase::handleAirFrame - forward the frame to the connected eNB/gNB" << endl;
            }
        }
        else {
            EV << "NtnPhyBase::handleAirFrame - no channel model configured for carrier "
               << carrierFrequency << " on " << (isFeederLink_ ? "feeder" : "service") << " link" << endl;
        }
    }
    else {
        // CONTROL packet
        EV << "NtnPhyBase::handleAirFrame - forward the control frame to " << ((nodeType_ == SATELLITE_NODE) ? (isFeederLink_ ? "the service link NIC for relaying" : "the feeder link NIC for relaying") : "the connected eNB/gNB") << endl;
    }

    send(msg, "upperLayerOut");
}

void NtnPhyBase::handleUpperMessage(cMessage *msg)
{
    if (isFeederLink_) {
        auto *frame = check_and_cast<LteAirFrame *>(msg);
        cModule *peerNode = resolvePeerNode();
        cGate *peerGate = resolvePeerGate();
        UserControlInfo lteInfo(frame->getAdditionalInfo());

        EV_DEBUG << "NtnPhyBase::handleUpperMessage - shifting carrier frequency from " << lteInfo.getCarrierFrequency() ;
        lteInfo.setCarrierFrequency(shiftFrequencyBand(lteInfo.getCarrierFrequency()));
        EV_DEBUG << " to " << lteInfo.getCarrierFrequency() << endl;

        inet::GeoCoord txWgs84 = referenceSystem_->wgs84FromOmnet(getRadioPosition());
        lteInfo.setRadioTransmitterId(nodeId_);
        lteInfo.setRadioTransmitterCoord(getRadioPosition());
        lteInfo.setRadioTransmitterEcefCoord(ecefFromWgs84(txWgs84));
        lteInfo.setRadioReceiverId(MacNodeId(peerNode->par("macNodeId").intValue()));
        lteInfo.setRadioTransmitterAntenna(antennaModel_);
        frame->setAdditionalInfo(lteInfo);
        EV << "NtnPhyBase::handleUpperMessage - forwarding air frame " << frame->getName() << " to peer node " << peerNode->getFullPath()
                << ", carrierFreq[" << lteInfo.getCarrierFrequency() << "]" << endl;
        sendDirect(frame, 0, frame->getDuration(), peerGate);
    }
    else {
        auto *frame = check_and_cast<LteAirFrame *>(msg);
        UserControlInfo lteInfo(frame->getAdditionalInfo());
        lteInfo.setCarrierFrequency(shiftFrequencyBand(lteInfo.getCarrierFrequency()));
        MacNodeId destId = lteInfo.getDestId();
        cModule *receiver = binder_->getNodeModule(destId);
        if (receiver == nullptr) {
            EV << "NtnPhyBase::handleUpperMessage - destination node " << destId << " is not available. Delete frame " << frame->getName() << endl;
            delete frame;
            return;
        }

        inet::GeoCoord txWgs84 = referenceSystem_->wgs84FromOmnet(getRadioPosition());
        lteInfo.setRadioTransmitterId(nodeId_);
        lteInfo.setRadioTransmitterCoord(getRadioPosition());
        lteInfo.setRadioTransmitterEcefCoord(ecefFromWgs84(txWgs84));
        lteInfo.setRadioReceiverId(destId);
        frame->setAdditionalInfo(lteInfo);
        EV << "NtnPhyBase::handleUpperMessage - forwarding air frame " << frame->getName() << " to node " << destId << endl;
        sendDirect(frame, 0, frame->getDuration(), receiver, getReceiverGateIndex(receiver, isNrUe(destId)));
    }
}

void NtnPhyBase::handleMessage(cMessage *msg)
{
    cGate *arrivalGate = msg->getArrivalGate();
    if (arrivalGate->isName("upperLayerIn")) {
        handleUpperMessage(msg);
        return;
    }

    if (arrivalGate->isName("radioIn")) {
        handleAirFrame(msg);
        return;
    }

    throw cRuntimeError("NtnPhyBase::handleMessage - unexpected gate %s", arrivalGate->getName());
}

cModule *NtnPhyBase::resolvePeerNode() const
{
    if (nodeType_ == NTN_GATEWAY_NODE) {
        MacNodeId satelliteId = binder_->getAssociatedSatelliteForGateway(nodeId_);
        SatelliteInfo *info = binder_->getSatelliteInfo(satelliteId);
        if (info == nullptr || info->satelliteModule == nullptr)
            throw cRuntimeError("NtnPhyBase::resolvePeerNode - satellite %hu is not registered", num(satelliteId));
        return info->satelliteModule.get();
    }

    if (nodeType_ == SATELLITE_NODE) {
        MacNodeId gatewayId = binder_->getAssociatedGatewayForSatellite(nodeId_);
        NtnGatewayInfo *info = binder_->getNtnGatewayInfo(gatewayId);
        if (info == nullptr || info->gatewayModule == nullptr)
            throw cRuntimeError("NtnPhyBase::resolvePeerNode - NTN gateway %hu is not registered", num(gatewayId));
        return info->gatewayModule.get();
    }

    throw cRuntimeError("NtnPhyBase::resolvePeerNode - unsupported NTN node type %s", nodeTypeToA(nodeType_));
}

cGate *NtnPhyBase::resolvePeerGate() const
{
    cModule *peerNode = resolvePeerNode();
    cGate *peerGate = peerNode->gate("feederLinkRadioIn");
    if (peerGate == nullptr)
        throw cRuntimeError("NtnPhyBase::resolvePeerGate - peer node %s has no feederLinkRadioIn gate", peerNode->getFullPath().c_str());
    return peerGate;
}

int NtnPhyBase::getReceiverGateIndex(const cModule *receiver, bool isNr) const
{
    int gate = isNr ? receiver->findGate("nrRadioIn") : receiver->findGate("radioIn");
    if (gate < 0) {
        gate = receiver->findGate("lteRadioIn");
        if (gate < 0)
            throw cRuntimeError("receiver \"%s\" has no suitable radio input gate", receiver->getFullPath().c_str());
    }
    return gate;
}

} // namespace simu5g
