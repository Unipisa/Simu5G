//
//                  Simu5G
//
// Copyright (C) 2026 Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef TESTS_UNIT_LIB_CHANNELMODELTESTUTIL_H_
#define TESTS_UNIT_LIB_CHANNELMODELTESTUTIL_H_

#include <map>

#include <omnetpp.h>

#include "simu5g/common/binder/Binder.h"
#include "simu5g/stack/phy/channelmodel/IRadioEndpoint.h"
#include "simu5g/stack/phy/channelmodel/PathLossModel.h"
#include "simu5g/stack/phy/channelmodel/StochasticChannelModel.h"
#include "simu5g/stack/phy/medium/CellularRadioMedium.h"
#include "simu5g/stack/phy/medium/CellularTransmission.h"
#include "simu5g/stack/phy/radio/BlerCurveErrorModel.h"
#include "simu5g/stack/phy/radio/CellularReceiver.h"

namespace simu5g {
namespace unittest {

/**
 * A radio endpoint that is only data: what a channel model asks a PHY for, set
 * directly by the test. This is what IRadioEndpoint exists for -- a channel
 * model can be put in front of endpoints like this one without a node, a NIC
 * or a PHY module around it.
 *
 * getChannelModel() reproduces PhyBase's lookup exactly, including returning
 * the first carrier's model for GHz(0.0), because the code under test relies on
 * that behaviour and a stub that differed would test something else.
 */
class StubEndpoint : public IRadioEndpoint
{
  public:
    MacNodeId nodeId = NODEID_NONE;
    inet::Coord coord;
    double speed = 0.0;
    bool insideBuilding = false;
    double insideDistance = 0.0;
    double penetrationLossDeviate = 0.0;
    TxDirectionType txDirection = OMNI;
    double txAngle = 0.0;
    double txPower = 0.0;
    double antennaGain = 0.0;
    double noiseFigure = 0.0;
    double cableLoss = 0.0;
    CellularReceiver *receiver = nullptr;
    std::map<GHz, ChannelModelBase *> channelModels;  // by carrier frequency

    MacNodeId getMacNodeId() override { return nodeId; }
    const inet::Coord& getCoord() const override { return coord; }
    double getSpeed() override { return speed; }
    bool isInsideBuilding() override { return insideBuilding; }
    double getInsideDistance() override { return insideDistance; }
    double getPenetrationLossDeviate() override { return penetrationLossDeviate; }
    TxDirectionType getTxDirection() override { return txDirection; }
    double getTxAngle() override { return txAngle; }
    double getTxPwr(Direction dir = UNKNOWN_DIRECTION) override { return txPower; }
    double getAntennaGain() override { return antennaGain; }
    double getNoiseFigure() override { return noiseFigure; }
    double getCableLoss() override { return cableLoss; }
    CellularReceiver *getReceiver() override { return receiver; }

    ChannelModelBase *getChannelModel(GHz carrierFreq = GHz(0.0)) override
    {
        if (channelModels.empty())
            return nullptr;
        if (carrierFreq == GHz(0.0))
            return channelModels.begin()->second;
        auto it = channelModels.find(carrierFreq);
        return it == channelModels.end() ? nullptr : it->second;
    }
};

/**
 * Registers a stub UE with the Binder through its real API -- addUeInfo(), the
 * call LteMacUe makes for a real one -- so that code which finds a UE by
 * walking the Binder's UE list finds this one.
 *
 * The Binder owns the record and deletes it at the end of the simulation; the
 * endpoint is the test's. When the registration goes out of scope it detaches
 * the record from the endpoint, so that nothing is left holding a pointer to a
 * stub that no longer exists. (Binder::finish() only reads the UE list when
 * printTrafficGeneratorConfig is set, which a harness must leave off: it would
 * cast the endpoint to a PhyUe.)
 */
class StubUeRegistration
{
  private:
    UeInfo *info_;

  public:
    StubUeRegistration(Binder *binder, MacNodeId id, StubEndpoint *endpoint)
        : info_(new UeInfo())
    {
        info_->init = false;
        info_->txPwr = 0.0;
        info_->id = id;
        info_->cellId = NODEID_NONE;
        info_->phy = endpoint;
        binder->addUeInfo(info_);
    }

    ~StubUeRegistration() { info_->phy = nullptr; }

    StubUeRegistration(const StubUeRegistration&) = delete;
    StubUeRegistration& operator=(const StubUeRegistration&) = delete;
};

/**
 * Registers a stub endpoint with the radio medium, as a PHY registers its
 * radio, for the lifetime of the object -- so that a channel model looking up
 * another node's radio through the medium finds the stub.
 */
class StubRadioRegistration
{
  private:
    CellularRadioMedium *medium_;
    IRadioEndpoint *endpoint_;

  public:
    StubRadioRegistration(CellularRadioMedium *medium, MacNodeId id, IRadioEndpoint *endpoint)
        : medium_(medium), endpoint_(endpoint)
    {
        medium_->addRadio(id, endpoint_);
    }

    ~StubRadioRegistration() { medium_->removeRadio(endpoint_); }

    StubRadioRegistration(const StubRadioRegistration&) = delete;
    StubRadioRegistration& operator=(const StubRadioRegistration&) = delete;
};

/**
 * The protected parts of StochasticChannelModel, for a test that has to look at
 * the model's state or call its internal steps directly.
 *
 * Access is never instantiated. A class derived from the model may name the
 * model's protected members, and a pointer-to-member formed through it has the
 * model as its class type -- so it can be applied to the model object the
 * network built, which is not an Access. This is plain C++: no friend
 * declaration in the model, and no cast of the object to a type it is not.
 */
class ChannelModelProbe
{
  private:
    struct Access : StochasticChannelModel
    {
        static auto losMapPtr() { return &Access::losMap; }
        static auto shadowingMapPtr() { return &Access::shadowingMap; }
        static auto jakesFadingMapPtr() { return &Access::jakesFadingMap; }
        static auto pathLossPtr() { return &Access::pathLoss_; }
        static auto fadingPtr() { return &Access::fading_; }

        static auto linkForPtr() { return &Access::linkFor; }
        static auto whatIfLinkPtr() { return &Access::whatIfLink; }
        static auto cellularLinkPtr() { return &Access::cellularLink; }
        static auto emitRcvdSinrPtr() { return &Access::emitRcvdSinr; }
        static auto phantomPowerAtPtr() { return &Access::phantomPowerAt; }
    };

    StochasticChannelModel *model_;

  public:
    explicit ChannelModelProbe(StochasticChannelModel *model) : model_(model) {}

    StochasticChannelModel *model() const { return model_; }

    // state, which the radio medium keeps
    // the LOS state of the links on the model's carrier, which the radio medium keeps once per link
    ChannelState::LosMap& losMap() { return (model_->*Access::losMapPtr())(); }
    // the shadowing of the links on the model's carrier, which the radio medium keeps once per link
    ChannelState::ShadowFadingMap& shadowingMap() { return (model_->*Access::shadowingMapPtr())(); }
    // the Jakes fading paths of the links on the model's carrier, which the radio medium keeps once per link
    ChannelState::JakesFadingMap& jakesMap() { return (model_->*Access::jakesFadingMapPtr())(); }
    PathLossModel *pathLoss() { return model_->*Access::pathLossPtr(); }
    bool& fading() { return model_->*Access::fadingPtr(); }

    // internal steps
    // the link of a frame from a sender at senderPosition that did not arrive through the radio the evaluation is at: the local end where the radio is now
    RadioLink linkFor(IRadioEndpoint *radio, UserControlInfo *info, const inet::Coord& senderPosition)
    {
        return (model_->*Access::linkForPtr())(radio, info, senderPosition, radio->getCoord());
    }
    // the link of a what-if: the transmission to where the receiver is now
    RadioLink whatIfLink(IRadioEndpoint *receiver, const CellularTransmission& transmission) { return (model_->*Access::whatIfLinkPtr())(receiver, transmission); }
    RadioLink cellularLink(IRadioEndpoint *radio, MacNodeId ueId, Direction dir, inet::Coord coord)
    {
        return (model_->*Access::cellularLinkPtr())(radio, ueId, dir, coord);
    }
    void emitRcvdSinr(IRadioEndpoint *radio, Direction dir, MacNodeId ueId, GHz carrierFrequency, double sinr)
    {
        (model_->*Access::emitRcvdSinrPtr())(radio, dir, ueId, carrierFrequency, sinr);
    }
    // what a phantom transmission (an external or a background cell's) adds at the receiver, in dBm
    double phantomPowerAt(const CellularTransmission& transmission, IRadioEndpoint *receiver)
    {
        return (model_->*Access::phantomPowerAtPtr())(transmission, receiver, receiver->getCoord());
    }
};

/**
 * A downlink CQI as the base station computes it from a UE's report, a
 * what-if: a transmission of the base station's radio to the UE, from where
 * the base station is, starting now.
 */
inline CellularTransmission dlCqiTransmission(IRadioEndpoint *bs, MacNodeId ueId)
{
    CellularTransmission transmission;
    transmission.transmitter = bs;
    transmission.sourceId = bs->getMacNodeId();
    transmission.destId = ueId;
    transmission.cellId = bs->getMacNodeId();
    transmission.direction = DL;
    transmission.frameType = FEEDBACKPKT;
    transmission.startPosition = bs->getCoord();
    transmission.startTime = omnetpp::simTime();
    return transmission;
}

/**
 * A copy of the random number generator a component draws from, taken at a
 * moment of the test's choosing. Drawing from the copy reproduces, in order,
 * the numbers the component will draw next. So a test can compute what a random
 * quantity ought to be from the very sample the model is about to use, and
 * grade a recurrence exactly instead of statistically -- and a test that draws
 * the wrong number of times from the copy will notice, because every value after
 * that point disagrees.
 */
class RngSnapshot
{
  private:
    omnetpp::cMersenneTwister twin_;

  public:
    explicit RngSnapshot(const omnetpp::cComponent *component, int k = 0)
        : twin_(*omnetpp::check_and_cast<omnetpp::cMersenneTwister *>(component->getRNG(k)))
    {
    }

    omnetpp::cRNG *rng() { return &twin_; }
};

/**
 * A submodule of the harness network the %activity driver is part of.
 */
template<typename T>
T *harnessModule(omnetpp::cModule *driver, const char *name)
{
    return omnetpp::check_and_cast<T *>(driver->getParentModule()->getSubmodule(name));
}

} // namespace unittest
} // namespace simu5g

#endif
