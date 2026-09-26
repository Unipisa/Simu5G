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

#include "simu5g/stack/d2d/phy/D2dUePhyHelper.h"

#include <inet/common/packet/Packet.h>

#include "simu5g/common/LteControlInfo.h"
#include "simu5g/common/LteControlInfoTags_m.h"
#include "simu5g/stack/phy/PhyBase.h"
#include "simu5g/stack/phy/channelmodel/ChannelModelBase.h"
#include "simu5g/stack/d2d/phy/channelmodel/ID2dChannelModel.h"
#include "simu5g/stack/phy/packet/LteAirFrame.h"

namespace simu5g {

using namespace inet;

void D2dUePhyHelper::initWatches()
{
    WATCH(nearestDistance_);
    WATCH(bestRsrpMean_);
    WATCH(bestRsrpVector_);
}

void D2dUePhyHelper::storeAirFrame(LteAirFrame *newFrame)
{
    // Implements the capture effect
    // Store the frame received from the nearest transmitter
    UserControlInfo *newInfo = check_and_cast<UserControlInfo *>(newFrame->getControlInfo());
    GHz carrierFreq = newInfo->getCarrierFrequency();
    ChannelModelBase *channelModel = phy_->getChannelModel(carrierFreq);
    if (channelModel == nullptr)
        throw cRuntimeError("D2dUePhyHelper::storeAirFrame - Carrier frequency [%f] not supported by any channel model", carrierFreq.get());

    Coord myCoord = phy_->getCoord();
    double distance = 0.0;
    double rsrpMean = 0.0;
    std::vector<double> rsrpVector;
    bool useRsrp = false;

    if (strcmp(phy_->par("d2dMulticastCaptureEffectFactor"), "RSRP") == 0) {
        useRsrp = true;

        double sum = 0.0;
        unsigned int allocatedRbs = 0;
        rsrpVector = check_and_cast<ID2dChannelModel *>(channelModel)->getRSRP_D2D(newFrame, newInfo, phy_->getMacNodeId(), myCoord);

        // Get the average RSRP on the RBs allocated for the transmission
        RbMap rbmap = newInfo->getGrantedBlocks();
        // For each Remote unit used to transmit the packet
        for (const auto &[remoteUnit, rbList] : rbmap) {
            // For each logical band used to transmit the packet
            for (const auto &[band, allocation] : rbList) {
                if (allocation == 0) // This Rb is not allocated
                    continue;

                sum += rsrpVector.at(band);
                allocatedRbs++;
            }
        }
        if (allocatedRbs > 0)
            rsrpMean = sum / allocatedRbs;
        EV << NOW << " D2dUePhyHelper::storeAirFrame - Average RSRP from node " << newInfo->getSourceId() << ": " << rsrpMean << endl;
    }
    else { // Distance
        Coord newSenderCoord = newInfo->getCoord();
        distance = myCoord.distance(newSenderCoord);
        EV << NOW << " D2dUePhyHelper::storeAirFrame - Distance from node " << newInfo->getSourceId() << ": " << distance << endl;
    }

    if (!d2dReceivedFrames_.empty()) {
        LteAirFrame *prevFrame = d2dReceivedFrames_.front();
        if (!useRsrp && distance < nearestDistance_) {
            EV << "[ < nearestDistance: " << nearestDistance_ << "]" << endl;

            // Remove the previous frame
            d2dReceivedFrames_.pop_back();
            delete prevFrame;

            nearestDistance_ = distance;
            d2dReceivedFrames_.push_back(newFrame);
        }
        else if (rsrpMean > bestRsrpMean_) {
            EV << "[ > bestRsrp: " << bestRsrpMean_ << "]" << endl;

            // Remove the previous frame
            d2dReceivedFrames_.pop_back();
            delete prevFrame;

            bestRsrpMean_ = rsrpMean;
            bestRsrpVector_ = rsrpVector;
            d2dReceivedFrames_.push_back(newFrame);
        }
        else {
            // This frame will not be decoded
            delete newFrame;
        }
    }
    else {
        if (!useRsrp) {
            nearestDistance_ = distance;
            d2dReceivedFrames_.push_back(newFrame);
        }
        else {
            bestRsrpMean_ = rsrpMean;
            bestRsrpVector_ = rsrpVector;
            d2dReceivedFrames_.push_back(newFrame);
        }
    }
}

LteAirFrame *D2dUePhyHelper::extractAirFrame()
{
    // Implements the capture effect
    // The vector is storing the frame received from the strongest/nearest transmitter

    return d2dReceivedFrames_.front();
}

void D2dUePhyHelper::decodeAirFrame(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    EV << NOW << " D2dUePhyHelper::decodeAirFrame - Start decoding..." << endl;

    GHz carrierFreq = lteInfo->getCarrierFrequency();
    ChannelModelBase *channelModel = phy_->getChannelModel(carrierFreq);
    if (channelModel == nullptr)
        throw cRuntimeError("D2dUePhyHelper::decodeAirFrame - Carrier frequency [%f] not supported by any channel model", carrierFreq.get());

    // Apply decider to received packet. D2D and D2D_MULTI no longer need their own
    // entry point: the core reception decision handles every direction, and
    // bestRsrpVector_ carries the capture-effect RSRP for the one-to-many case.
    bool result = channelModel->isReceptionSuccessful(frame, lteInfo, bestRsrpVector_);

    EV << "Handled LteAirframe with ID " << frame->getId() << " with result "
       << (result ? "RECEIVED" : "NOT RECEIVED") << endl;

    auto pkt = check_and_cast<inet::Packet *>(frame->decapsulate());

    // Note: no need to delete the frame itself - will be deleted later when the buffer of
    // received frames is cleared

    // Attach the decider result to the packet as control info
    *(pkt->addTagIfAbsent<UserControlInfo>()) = *lteInfo;
    delete lteInfo;

    // Send the decapsulated packet up (updates stats and display string)
    phy_->sendDecodedPacketUp(pkt, result);
}

void D2dUePhyHelper::decodeStoredFrames()
{
    // Select one frame from the buffer. Implements the capture effect.
    LteAirFrame *frame = extractAirFrame();
    UserControlInfo *lteInfo = check_and_cast<UserControlInfo *>(frame->removeControlInfo());

    // Decode the selected frame.
    decodeAirFrame(frame, lteInfo);

    // Clear buffer.
    while (!d2dReceivedFrames_.empty()) {
        LteAirFrame *f = d2dReceivedFrames_.back();
        d2dReceivedFrames_.pop_back();
        delete f;
    }
}

} //namespace
