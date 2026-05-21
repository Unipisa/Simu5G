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

#include "simu5g/stack/phy/NtnPhyGnb.h"

#include "simu5g/common/LteControlInfoTags_m.h"
#include "simu5g/stack/phy/packet/NtnAirFrame.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

Define_Module(NtnPhyGnb);

void NtnPhyGnb::initialize(int stage)
{
    LtePhyEnb::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        ntnInGate_ = gate("ntnIn");
        ntnOutGate_ = gate("ntnOut");
        isNr_ = true;
    }
}

LteAirFrame *NtnPhyGnb::createAirFrame(const char *name)
{
    return new NtnAirFrame(name);
}

void NtnPhyGnb::handleMessage(cMessage *msg)
{
    if (msg->getArrivalGate() == ntnInGate_) {
        handleNtnAirFrame(msg);
        return;
    }

    LtePhyEnb::handleMessage(msg);
}

void NtnPhyGnb::sendNtn(LteAirFrame *airFrame)
{
    if (ntnOutGate_ == nullptr || !ntnOutGate_->isConnected())
        throw cRuntimeError("NtnPhyGnb::sendNtn - NTN fronthaul gate is not connected for %s", getFullPath().c_str());

    if (airFrame->getControlInfo() != nullptr) {
        UserControlInfo *userControlInfo = check_and_cast<UserControlInfo *>(airFrame->removeControlInfo());
        airFrame->setAdditionalInfo(*userControlInfo);
        delete userControlInfo;
    }

    send(airFrame, ntnOutGate_);
}

void NtnPhyGnb::sendBroadcast(LteAirFrame *airFrame)
{
    sendNtn(airFrame);
}

void NtnPhyGnb::sendUnicast(LteAirFrame *airFrame)
{
    sendNtn(airFrame);
}

void NtnPhyGnb::handleNtnAirFrame(cMessage *msg)
{
    LteAirFrame *frame = static_cast<LteAirFrame *>(msg);
    UserControlInfo *lteInfo = new UserControlInfo(frame->getAdditionalInfo());

    EV << "NtnPhyGnb::handleNtnAirFrame - received new LteAirFrame with ID " << frame->getId() << " from NTN gateway" << endl;

    if (lteInfo->getFrameType() == HANDOVERPKT) {
        EV << "NtnPhyGnb::handleNtnAirFrame - received handover packet from NTN gateway. Ignore it." << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    MacNodeId sourceId = lteInfo->getSourceId();
    if (binder_->getNextHop(sourceId) != nodeId_) {
        EV << "WARNING: frame from a UE that is leaving this cell (handover): deleted " << endl;
        EV << "Source MacNodeId: " << sourceId << endl;
        EV << "Master MacNodeId: " << nodeId_ << endl;
        delete lteInfo;
        delete frame;
        return;
    }

    if (!binder_->nodeExists(sourceId) || !binder_->nodeExists(lteInfo->getDestId())) {
        delete lteInfo;
        delete frame;
        return;
    }

    if (handleControlPkt(lteInfo, frame))
        return;

    auto *ntnFrame = dynamic_cast<NtnAirFrame *>(frame);
    if (ntnFrame == nullptr)
        throw cRuntimeError("NtnPhyGnb::handleNtnAirFrame - frame %s from NTN gateway is not an NtnAirFrame", frame->getFullName());
    if (!ntnFrame->hasGatewayReceptionResult())
        throw cRuntimeError("NtnPhyGnb::handleNtnAirFrame - NtnAirFrame %s from NTN gateway has no gateway reception result", frame->getFullName());

    // The NTN gateway already made the feeder/service-link reception decision.
    bool result = ntnFrame->getGatewayReceptionResult();
    sendDecodedDataFrame(frame, lteInfo, result);
}

LteAirFrame *NtnPhyGnb::createCsiReferenceSignalFrame(GHz carrierFrequency)
{
    LteAirFrame *csiAirFrame = new NtnAirFrame("CsiReferenceSignal");
    UserControlInfo *cInfo = new UserControlInfo();
    cInfo->setSourceId(nodeId_);
    cInfo->setFrameType(CSIRSPKT);
    cInfo->setDirection(DL);
    cInfo->setTxPower(txPower_);
    cInfo->setCarrierFrequency(carrierFrequency);
    cInfo->setIsNr(isNr_);
    cInfo->setCoord(getRadioPosition());
    csiAirFrame->setControlInfo(cInfo);
    csiAirFrame->setDuration(TTI);
    csiAirFrame->setSchedulingPriority(airFramePriority_);
    return csiAirFrame;
}

void NtnPhyGnb::sendCsiReferenceSignalFrameToAttachedUes(LteAirFrame *frame)
{
    if (ntnOutGate_ == nullptr || !ntnOutGate_->isConnected())
        throw cRuntimeError("NtnPhyGnb::sendCsiReferenceSignalFrameToAttachedUes - NTN fronthaul gate is not connected for %s", getFullPath().c_str());

    auto *ntnFrame = dynamic_cast<NtnAirFrame *>(frame);
    if (ntnFrame == nullptr)
        throw cRuntimeError("NtnPhyGnb::sendCsiReferenceSignalFrameToAttachedUes - CSI-RS frame %s is not an NtnAirFrame", frame->getFullName());

    UserControlInfo *ci = check_and_cast<UserControlInfo *>(frame->getControlInfo());
    frame->setAdditionalInfo(*ci);
    delete frame->removeControlInfo();

    std::vector<MacNodeId> attachedUes;
    for (MacNodeId ueId : cellInfo_->getAttachedUes()) {
        if (isNrUe(ueId) == isNr_)
            attachedUes.push_back(ueId);
    }

    ntnFrame->setAttachedUesVector(attachedUes);
    EV << "NtnPhyGnb::sendCsiReferenceSignalFrameToAttachedUes - sending CSI-RS frame with "
       << attachedUes.size() << " attached UE target(s) via NTN" << endl;
    send(frame, ntnOutGate_);
}

} // namespace simu5g
