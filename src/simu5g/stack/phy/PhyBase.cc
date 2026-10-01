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

#include "simu5g/stack/phy/PhyBase.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfoTags_m.h"
#include "simu5g/stack/mac/LteMacEnb.h"
#include "simu5g/stack/phy/radio/CellularAntenna.h"
#include "simu5g/stack/phy/radio/CellularRadio.h"
#include "simu5g/stack/phy/radio/CellularReceiver.h"
#include "simu5g/stack/phy/radio/RadioTransmissionRequest.h"

#include <inet/common/InitStages.h>
#include <inet/common/ModuleAccess.h>
#include <inet/mobility/contract/IMobility.h>

namespace inet {
// this is needed to ensure the correct ordering among initialization stages
// this should be fixed directly in INET
Define_InitStage_Dependency(PHYSICAL_LAYER, SINGLE_MOBILITY);
} // namespace inet

namespace simu5g {

using namespace omnetpp;

short PhyBase::airFramePriority_ = 10;

//Statistics
simsignal_t PhyBase::averageCqiDlSignal_ = registerSignal("averageCqiDl");
simsignal_t PhyBase::averageCqiUlSignal_ = registerSignal("averageCqiUl");

static int parseInt(const char *s, int defaultValue)
{
    if (!s || !*s)
        return defaultValue;

    char *endptr;
    int value = strtol(s, &endptr, 10);
    return *endptr == '\0' ? value : defaultValue;
}

PhyBase::~PhyBase()
{
    // radioMedium_ is unset if the medium has already been deleted
    if (radioMedium_ && registeredWithMedium_)
        radioMedium_->removeRadio(this);
}

void PhyBase::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        radioMedium_.reference(this, "radioMediumModule", true);
        hostModule_ = inet::getContainingNode(this);
        // register to get a notification when position changes
        if (hostModule_->findSubmodule("mobility") != -1)
            hostModule_->subscribe(inet::IMobility::mobilityStateChangedSignal, this);

        WATCH(radioPos_);
        WATCH(positionUpdateArrived_);

        binder_.reference(this, "binderModule", true);
        // get gate ids
        upperGateIn_ = findGate("upperGateIn");
        upperGateOut_ = findGate("upperGateOut");
        radioInGate_ = findGate("radioIn");
        radioOutGate_ = findGate("radioOut");
        radio_ = getModuleByPath(par("radioModule"));
        check_and_cast<CellularRadio *>(radio_.get())->setEndpoint(this);

        // Initialize and watch statistics
        ueTxPower_ = par("ueTxPower");
        eNodeBtxPower_ = par("eNodeBTxPower");
        microTxPower_ = par("microTxPower");
        isNr_ = par("isNr");

        WATCH(numAirFrameReceived_);
        WATCH(numAirFrameNotReceived_);
    }
    else if (stage == INITSTAGE_SIMU5G_REGISTRATIONS) {
        // a PHY without a node id is on the medium, but cannot be looked up by id
        radioMedium_->addRadio(nodeId_, this, radio_);
        registeredWithMedium_ = true;
    }
    else if (stage == inet::INITSTAGE_SINGLE_MOBILITY) {
        if (!positionUpdateArrived_ && hostModule_->isSubscribed(inet::IMobility::mobilityStateChangedSignal, this)) {
            // ...else, get the initial position from the display string
            radioPos_.x = parseInt(hostModule_->getDisplayString().getTagArg("p", 0), -1);
            radioPos_.y = parseInt(hostModule_->getDisplayString().getTagArg("p", 1), -1);

            if (radioPos_.x == -1 || radioPos_.y == -1)
                throw cRuntimeError("The coordinates of '%s' host are invalid. Please set coordinates in "
                      "'@display' attribute, or configure Mobility for this host.",
                        hostModule_->getFullPath().c_str());

            const char *s = hostModule_->getDisplayString().getTagArg("p", 2);
            if (s != nullptr && *s)
                throw cRuntimeError("The coordinates of '%s' host are invalid. Please remove automatic arrangement"
                      " (3rd argument of 'p' tag)"
                      " from '@display' attribute, or configure Mobility for this host.",
                        hostModule_->getFullPath().c_str());
        }
    }
    else if (stage == INITSTAGE_SIMU5G_REGISTRATIONS2) {
        initializeChannelModel();
    }
}

bool PhyBase::isInsideBuilding()
{
    return primaryChannelModel_ != nullptr && primaryChannelModel_->isInsideBuilding();
}

double PhyBase::getInsideDistance()
{
    return primaryChannelModel_ != nullptr ? primaryChannelModel_->getInsideDistance() : 0.0;
}

void PhyBase::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *)
{
    // since background UEs and their mobility modules are submodules of the e/gNB, a mobilityStateChangedSignal
    // intended for a background UE would be intercepted by the e/gNB too, making it change its position.
    // To prevent this issue, we need to check if the source of the signal is the same as the module receiving it
    if (hostModule_ != source->getParentModule())
        return;

    if (signalID == inet::IMobility::mobilityStateChangedSignal) {
        inet::IMobility *mobility = check_and_cast<inet::IMobility *>(obj);
        radioPos_ = mobility->getCurrentPosition();
        positionUpdateArrived_ = true;

        // emit serving cell and the distance from it
        emitMobilityStats();
    }
}

void PhyBase::handleMessage(cMessage *msg)
{
    EV << "PhyBase::handleMessage - new message received" << endl;

    if (msg->isSelfMessage()) {
        handleSelfMessage(msg);
    }
    // AirFrame
    else if (msg->getArrivalGate()->getId() == radioInGate_) {
        handleAirFrame(msg);
    }
    // message from stack
    else if (msg->getArrivalGate()->getId() == upperGateIn_) {
        handleUpperMessage(msg);
    }
    // unknown message
    else {
        EV << "Unknown message received." << endl;
        delete msg;
    }
}

void PhyBase::handleControlMsg(AirFrame *frame,
        UserControlInfo *userInfo)
{
    auto pkt = check_and_cast<inet::Packet *>(frame->decapsulate());
    delete frame;
    *(pkt->addTagIfAbsent<UserControlInfo>()) = *userInfo;
    delete userInfo;
    send(pkt, upperGateOut_);
}

void PhyBase::sendDecodedPacketUp(inet::Packet *pkt, bool receptionSuccessful)
{
    // Update statistics
    if (receptionSuccessful)
        numAirFrameReceived_++;
    else
        numAirFrameNotReceived_++;

    pkt->addTagIfAbsent<PhyReceptionInd>()->setDeciderResult(receptionSuccessful);

    // Send decapsulated message along with result control info to upperGateOut_
    send(pkt, upperGateOut_);

    if (getEnvir()->isGUI())
        updateDisplayString();
}

void PhyBase::handleUpperMessage(cMessage *msg)
{
    EV << "Phy: message from stack" << endl;

    auto pkt = check_and_cast<inet::Packet *>(msg);
    auto lteInfo = pkt->removeTag<UserControlInfo>();

    AirFrame *frame = new AirFrame(airFrameNameFor(lteInfo.get()));

    frame->encapsulate(check_and_cast<cPacket *>(msg));

    // initialize frame fields
    frame->setSchedulingPriority(airFramePriorityFor(lteInfo.get()));

    // set transmission duration according to the numerology
    NumerologyIndex numerologyIndex = binder_->getNumerologyIndexFromCarrierFreq(lteInfo->getCarrierFrequency());
    double slotDuration = binder_->getSlotDurationFromNumerologyIndex(numerologyIndex);

    // set current position
    lteInfo->setCoord(getCoord());
    lteInfo->setTxPower(txPower_);
    stampExtraTxControlInfo(lteInfo.get());
    frame->setControlInfo(lteInfo.get()->dup());

    EV << "Phy: " << nodeTypeToA(nodeType_) << " with id " << nodeId_
       << " sending message to the air channel. Dest=" << lteInfo->getDestId() << endl;
    transmitFrame(frame, lteInfo.get(), slotDuration);
}

const char *PhyBase::airFrameNameFor(const UserControlInfo *info)
{
    switch (info->getFrameType()) {
        case HARQPKT: return "harqFeedback";
        case GRANTPKT: return "harqFeedback-grant";
        case RACPKT: return "rac";
        default: return "airframe";
    }
}

void PhyBase::transmitFrame(AirFrame *frame, const UserControlInfo *info, simtime_t duration)
{
    sendUnicast(frame, duration);
}

void PhyBase::initializeChannelModel()
{
    primaryChannelModel_.reference(this, "channelModelModule", true);
    primaryChannelModel_->setPhy(this);
    GHz carrierFreq = primaryChannelModel_->getCarrierFrequency();
    unsigned int numerologyIndex = primaryChannelModel_->getNumerologyIndex();
    channelModel_[carrierFreq] = primaryChannelModel_;

    if (nodeType_ == UE)
        binder_->registerCarrierUe(carrierFreq, numerologyIndex, nodeId_);

    int numChannelModels = primaryChannelModel_->getVectorSize();
    for (int index = 1; index < numChannelModels; index++) {
        ChannelModelBase *chanModel = check_and_cast<ChannelModelBase *>(primaryChannelModel_->getParentModule()->getSubmodule(primaryChannelModel_->getName(), index));
        chanModel->setPhy(this);
        GHz carrierFreq = chanModel->getCarrierFrequency();
        unsigned int numerologyIndex = chanModel->getNumerologyIndex();
        channelModel_[carrierFreq] = chanModel;
        if (nodeType_ == UE)
            binder_->registerCarrierUe(carrierFreq, numerologyIndex, nodeId_);
    }

    std::vector<GHz> carrierFrequencies;
    for (const auto& [carrierFrequency, channelModel] : channelModel_)
        carrierFrequencies.push_back(carrierFrequency);
    getReceiver()->setCarrierFrequencies(carrierFrequencies);
}

void PhyBase::updateDisplayString()
{
    char buf[80] = "";
    if (numAirFrameReceived_ > 0)
        sprintf(buf + strlen(buf), "af_ok:%d ", numAirFrameReceived_);
    if (numAirFrameNotReceived_ > 0)
        sprintf(buf + strlen(buf), "af_no:%d ", numAirFrameNotReceived_);
    getDisplayString().setTagArg("t", 0, buf);
}

void PhyBase::sendBroadcast(AirFrame *airFrame, simtime_t duration)
{
    // Remove control info to allow parsim packing
    if (airFrame->getControlInfo() != nullptr) {
        UserControlInfo *userControlInfo = check_and_cast<UserControlInfo *>(airFrame->removeControlInfo());
        airFrame->setAdditionalInfo(*userControlInfo);
        delete userControlInfo;
    }

    // the radio has the medium deliver it to the radios in range
    auto request = new RadioTransmissionRequest();
    request->broadcast = true;
    request->sender = this;
    request->cellId = getCellId();
    request->duration = duration;
    airFrame->setControlInfo(request);
    send(airFrame, radioOutGate_);
}

void PhyBase::sendUnicast(AirFrame *frame, simtime_t duration)
{
    UserControlInfo *ci = check_and_cast<UserControlInfo *>(
            frame->getControlInfo());
    // dest MacNodeId from control info
    MacNodeId dest = ci->getDestId();
    cModule *receiver = binder_->getNodeModule(dest);
    if (receiver == nullptr) {
        // destination node has left the simulation
        delete frame;
        return;
    }

    // Remove control info to allow parsim packing
    if (frame->getControlInfo() != nullptr) {
        UserControlInfo *userControlInfo = check_and_cast<UserControlInfo *>(frame->removeControlInfo());
        frame->setAdditionalInfo(*userControlInfo);
        delete userControlInfo;
    }

    // the radio sends it to the receiving node's radio
    auto request = new RadioTransmissionRequest();
    request->sender = this;
    request->cellId = getCellId();
    request->targets.push_back(receiver->gate(getReceiverGateIndex(receiver, dest)));
    request->duration = duration;
    frame->setControlInfo(request);
    send(frame, radioOutGate_);
}

TxDirectionType PhyBase::getTxDirection()
{
    return check_and_cast<CellularAntenna *>(radio_->getSubmodule("antenna"))->getTxDirection();
}

double PhyBase::getTxAngle()
{
    return check_and_cast<CellularAntenna *>(radio_->getSubmodule("antenna"))->getTxAngle();
}

double PhyBase::getAntennaGain()
{
    return check_and_cast<CellularAntenna *>(radio_->getSubmodule("antenna"))->getGain();
}

double PhyBase::getNoiseFigure()
{
    return getReceiver()->getNoiseFigure();
}

double PhyBase::getCableLoss()
{
    return getReceiver()->getCableLoss();
}

CellularReceiver *PhyBase::getReceiver()
{
    return check_and_cast<CellularReceiver *>(radio_->getSubmodule("receiver"));
}

int PhyBase::getReceiverGateIndex(const cModule *receiver, MacNodeId dest) const
{
    int gate = isNrUe(dest) ? receiver->findGate("nrRadioIn") : receiver->findGate("radioIn");
    if (gate < 0) {
        gate = receiver->findGate("lteRadioIn");
        if (gate < 0) {
            throw cRuntimeError("receiver \"%s\" has no suitable radio input gate",
                    receiver->getFullPath().c_str());
        }
    }
    return gate;
}

} //namespace
