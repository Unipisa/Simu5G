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

#include "simu5g/stack/phy/NtnPhyBase.h"

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
        if (referenceSystem_ == nullptr)
            throw cRuntimeError("NtnPhyBase::initialize - %s found no GeographicReferenceSystem module in the "
                    "network. A transparent NTN path cannot place its radios without one.", getFullPath().c_str());
        isFeederLink_ = par("linkType").stdstringValue() == "feeder";
        feederLinkFrequencyOffset_ = GHz(par("feederLinkFrequencyOffset"));
        cModule *node = getContainingNode(this);
        nodeId_ = MacNodeId(node->par("macNodeId").intValue());
        nodeType_ = aToNodeType(node->par("nodeType").stdstringValue());
        propagationDelay_.initialize(this, referenceSystem_);
    }
    else if (stage == INITSTAGE_SIMU5G_REGISTRATIONS2) {
        initializeChannelModels();
    }
}

void NtnPhyBase::initializeChannelModels()
{
    primaryChannelModel_.reference(this, "channelModelModule", true);
    primaryChannelModel_->setPhy(this);
    registerChannelModel(primaryChannelModel_);

    int numChannelModels = primaryChannelModel_->getVectorSize();
    for (int index = 1; index < numChannelModels; index++) {
        LteChannelModel *chanModel = check_and_cast<LteChannelModel *>(primaryChannelModel_->getParentModule()->getSubmodule(primaryChannelModel_->getName(), index));
        chanModel->setPhy(this);
        registerChannelModel(chanModel);
    }
}

void NtnPhyBase::registerChannelModel(LteChannelModel *channelModel)
{
    // A feeder-link NIC only ever evaluates the frequency-translated hop, so retune the
    // channel model itself rather than only offsetting the lookup key. Every
    // frequency-dependent term is computed inside the model from its own carrier and not
    // from the frame being evaluated: path loss, clutter and fading band selection,
    // building penetration, atmospheric absorption, scintillation, resource-block centre
    // frequencies, and Doppler. Offsetting only the key leaves all of those on the service
    // carrier, which understates feeder path loss by 20*log10(f_feeder / f_service).
    //
    // This runs at INITSTAGE_SIMU5G_REGISTRATIONS2, i.e. after LteChannelModel::initialize()
    // has read the component carrier and registered it with the CellInfo, so a cell is
    // still registered on the service carrier rather than on the translated one.
    if (isFeederLink_)
        channelModel->setCarrierFrequency(shiftFrequencyBand(channelModel->getCarrierFrequency()));

    channelModel_[channelModel->getCarrierFrequency()] = channelModel;
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

NtnPhyBase::HopAction NtnPhyBase::getHopAction(const UserControlInfo& lteInfo) const
{
    // The satellite is always the first radio receiver of a transparent path: downlink
    // frames reach it from the gateway over the feeder link, uplink frames from the UE
    // over the service link.
    if (nodeType_ == SATELLITE_NODE) {
        switch (lteInfo.getFrameType()) {
            case DATAPKT:
                return HopAction::STORE_RELAY_HOP_SINR;
            case CSIRSPKT:  // downlink reference signal, arrives from the gateway
                if (!isFeederLink_)
                    throw cRuntimeError("NtnPhyBase::getHopAction - satellite PHY %s received CSI-RS over the service link, but expected the feeder link (sourceId=%d, destId=%d)",
                            getFullPath().c_str(), lteInfo.getSourceId(), lteInfo.getDestId());
                return HopAction::STORE_RELAY_HOP_SINR;
            case SRSPKT:    // uplink reference signal, arrives from the UE
                if (isFeederLink_)
                    throw cRuntimeError("NtnPhyBase::getHopAction - satellite PHY %s received SRS over the feeder link, but expected the service link (sourceId=%d, destId=%d)",
                            getFullPath().c_str(), lteInfo.getSourceId(), lteInfo.getDestId());
                return HopAction::STORE_RELAY_HOP_SINR;
            case BEACONPKT: // downlink measurement signal, arrives from the gateway
                if (!isFeederLink_)
                    throw cRuntimeError("NtnPhyBase::getHopAction - satellite PHY %s received a beacon over the service link, but expected the feeder link (sourceId=%d, destId=%d)",
                            getFullPath().c_str(), lteInfo.getSourceId(), lteInfo.getDestId());
                return HopAction::STORE_RELAY_HOP_RSRP;
            default:
                return HopAction::RELAY_ONLY;
        }
    }

    // The gateway only receives over the radio on its feeder link, so every frame that
    // gets here is the last hop of an uplink. Downlink frames reach the gateway from the
    // gNB over the wired fronthaul and never pass through handleAirFrame().
    if (nodeType_ == NTN_GATEWAY_NODE && isFeederLink_) {
        switch (lteInfo.getFrameType()) {
            case DATAPKT:
                return HopAction::STORE_RECEPTION_RESULT;
            case SRSPKT:
                return HopAction::STORE_END_TO_END_SINR;
            default:
                return HopAction::RELAY_ONLY;
        }
    }

    return HopAction::RELAY_ONLY;
}

GHz NtnPhyBase::toServiceLinkCarrier(GHz carrierFreq) const
{
    if (!isFeederLink_ || std::isnan(carrierFreq.get()))
        return carrierFreq;

    double serviceFrequency = carrierFreq.get() - feederLinkFrequencyOffset_.get();
    if (serviceFrequency <= 0.0)
        throw cRuntimeError("NtnPhyBase::toServiceLinkCarrier - cannot translate feeder-link carrier %gGHz with offset %gGHz",
                carrierFreq.get(), feederLinkFrequencyOffset_.get());
    return GHz(serviceFrequency);
}

void NtnPhyBase::handleAirFrame(cMessage *msg)
{
    auto *frame = check_and_cast<LteAirFrame *>(msg);
    UserControlInfo lteInfo(frame->getAdditionalInfo());
    EV << "NtnPhyBase::handleAirFrame - received air frame " << msg->getName() << " from " << (isFeederLink_ ? "feeder" : "service") << " link radio"
            << ", carrierFreq[" << lteInfo.getCarrierFrequency() << "]" << endl;

    HopAction action = getHopAction(lteInfo);
    GHz carrierFrequency = lteInfo.getCarrierFrequency();
    LteChannelModel *channelModel = action == HopAction::RELAY_ONLY ? nullptr : getChannelModel(carrierFrequency);

    // Relaying the frame unevaluated would leave the first hop unmeasured, silently reducing
    // the transparent path to a single hop at the final receiver.
    if (action != HopAction::RELAY_ONLY && channelModel == nullptr)
        throw cRuntimeError("NtnPhyBase::handleAirFrame - no channel model configured for carrier %gGHz on the %s link of %s. "
                "This hop cannot be evaluated, which would silently reduce the transparent path to a single hop.",
                carrierFrequency.get(), isFeederLink_ ? "feeder" : "service", getFullPath().c_str());

    if (action != HopAction::RELAY_ONLY) {
        auto *ntnFrame = dynamic_cast<NtnAirFrame *>(frame);
        if (ntnFrame == nullptr)
            throw cRuntimeError("NtnPhyBase::handleAirFrame - transparent NTN frame %s is not an NtnAirFrame", frame->getFullName());

        switch (action) {
            case HopAction::STORE_RELAY_HOP_SINR:
                ntnFrame->setRelayHopSinrVector(channelModel->getSINR(frame, &lteInfo));
                break;
            case HopAction::STORE_RELAY_HOP_RSRP:
                ntnFrame->setRelayHopRsrpVector(channelModel->getRSRP(frame, &lteInfo));
                break;
            case HopAction::STORE_END_TO_END_SINR: {
                // Both hops combined, so the terrestrial gNB can derive uplink CSI from the
                // actual satellite path instead of measuring a channel it does not have.
                std::vector<double> sinrVector = channelModel->computeReceptionSinr(frame, &lteInfo);
                ntnFrame->setEndToEndSinrVector(sinrVector);
                EV << "NtnPhyBase::handleAirFrame - stored end-to-end SINR for frame " << frame->getName()
                   << " over " << sinrVector.size() << " band(s)" << endl;
                break;
            }
            case HopAction::STORE_RECEPTION_RESULT: {
                bool result = channelModel->isReceptionSuccessful(frame, &lteInfo);
                ntnFrame->setGatewayReceptionResultInfo(result);
                EV << "NtnPhyBase::handleAirFrame - handled LteAirframe with ID " << frame->getId() << " with result " << (result ? "RECEIVED" : "NOT RECEIVED") << endl;
                break;
            }
            case HopAction::RELAY_ONLY:
                break;
        }
    }

    if (nodeType_ == SATELLITE_NODE)
        EV << "NtnPhyBase::handleAirFrame - forward the frame to the " << (isFeederLink_ ? "service" : "feeder") << " NIC for relaying" << endl;
    else
        EV << "NtnPhyBase::handleAirFrame - forward the frame to the connected eNB/gNB" << endl;

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

        MacNodeId peerId = MacNodeId(peerNode->par("macNodeId").intValue());
        setRadioTransmitterInfo(lteInfo);
        lteInfo.setRadioReceiverId(peerId);
        frame->setAdditionalInfo(lteInfo);

        simtime_t delay = propagationDelay_.computeHopDelay(getRadioPosition(), peerGate, nodeId_, peerId);
        EV << "NtnPhyBase::handleUpperMessage - forwarding air frame " << frame->getName() << " to peer node " << peerNode->getFullPath()
                << ", carrierFreq[" << lteInfo.getCarrierFrequency() << "]"
                << ", propagationDelay[" << delay << "]" << endl;
        sendDirect(frame, delay, propagationDelay_.transmissionDuration(nodeType_, frame->getDuration()), peerGate);
    }
    else {
        auto *frame = check_and_cast<LteAirFrame *>(msg);
        UserControlInfo lteInfo(frame->getAdditionalInfo());
        lteInfo.setCarrierFrequency(shiftFrequencyBand(lteInfo.getCarrierFrequency()));
        auto *ntnFrame = dynamic_cast<NtnAirFrame *>(frame);

        if ((lteInfo.getFrameType() == CSIRSPKT || lteInfo.getFrameType() == BEACONPKT) && ntnFrame != nullptr && ntnFrame->hasAttachedUes()) {
            setRadioTransmitterInfo(lteInfo);

            std::vector<MacNodeId> attachedUes = ntnFrame->getAttachedUesVector();
            simtime_t duration = propagationDelay_.transmissionDuration(nodeType_, frame->getDuration());
            EV << "NtnPhyBase::handleUpperMessage - forwarding " << frame->getName()
               << " to " << attachedUes.size() << " attached UE target(s)" << endl;

            for (MacNodeId ueId : attachedUes) {
                cModule *receiver = binder_->getNodeModule(ueId);
                if (receiver == nullptr) {
                    EV << "NtnPhyBase::handleUpperMessage - attached UE " << ueId << " is not available. Skip copy." << endl;
                    continue;
                }

                int receiverGateId = getReceiverGateIndex(receiver, isNrUe(ueId));
                LteAirFrame *frameToSend = frame->dup();
                UserControlInfo ueInfo(lteInfo);
                ueInfo.setDestId(ueId);
                ueInfo.setRadioReceiverId(ueId);
                frameToSend->setAdditionalInfo(ueInfo);

                // The transmitter metadata is per-transmitter and stays hoisted out of the
                // loop, but the delay is per-receiver: this is the differential delay across
                // the beam, not a repeated computation of the same value.
                simtime_t delay = propagationDelay_.computeHopDelay(getRadioPosition(),
                        receiver->gate(receiverGateId), nodeId_, ueId);
                sendDirect(frameToSend, delay, duration, receiver, receiverGateId);
            }

            delete frame;
            return;
        }

        MacNodeId destId = lteInfo.getDestId();
        cModule *receiver = binder_->getNodeModule(destId);
        if (receiver == nullptr) {
            EV << "NtnPhyBase::handleUpperMessage - destination node " << destId << " is not available. Delete frame " << frame->getName() << endl;
            delete frame;
            return;
        }

        int receiverGateId = getReceiverGateIndex(receiver, isNrUe(destId));
        setRadioTransmitterInfo(lteInfo);
        lteInfo.setRadioReceiverId(destId);
        frame->setAdditionalInfo(lteInfo);

        simtime_t delay = propagationDelay_.computeHopDelay(getRadioPosition(),
                receiver->gate(receiverGateId), nodeId_, destId);
        EV << "NtnPhyBase::handleUpperMessage - forwarding air frame " << frame->getName() << " to node " << destId
           << ", propagationDelay[" << delay << "]" << endl;
        sendDirect(frame, delay, propagationDelay_.transmissionDuration(nodeType_, frame->getDuration()),
                receiver, receiverGateId);
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

inet::Coord NtnPhyBase::setRadioTransmitterInfo(UserControlInfo& lteInfo) const
{
    inet::Coord txEcef = propagationDelay_.ecefFromRadioPosition(getRadioPosition());
    lteInfo.setRadioTransmitterId(nodeId_);
    lteInfo.setRadioTransmitterCoord(getRadioPosition());
    lteInfo.setRadioTransmitterEcefCoord(txEcef);
    lteInfo.setRadioTransmitterAntenna(antennaModel_);
    lteInfo.setTxPower(antennaModel_->getTxPower());
    return txEcef;
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
