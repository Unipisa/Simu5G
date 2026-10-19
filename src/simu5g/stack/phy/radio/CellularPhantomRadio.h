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

#ifndef STACK_PHY_RADIO_CELLULARPHANTOMRADIO_H_
#define STACK_PHY_RADIO_CELLULARPHANTOMRADIO_H_

#include <omnetpp.h>
#include <inet/common/ModuleRefByPar.h>
#include <inet/mobility/contract/IMobility.h>

#include "simu5g/stack/phy/channelmodel/IRadioEndpoint.h"
#include "simu5g/stack/phy/medium/CellularRadioMedium.h"

namespace simu5g {

using namespace omnetpp;

class CellularAntenna;

/**
 * See the NED documentation of CellularPhantomRadio. The radio is its own
 * endpoint: it answers for itself what a PHY answers for a node's radio.
 * Outdoors, always.
 */
class CellularPhantomRadio : public cModule, public IRadioEndpoint
{
  protected:
    MacNodeId nodeId_ = NODEID_NONE;
    inet::ModuleRefByPar<CellularRadioMedium> radioMedium_;
    inet::ModuleRefByPar<inet::IMobility> mobility_; // nullptr if the node has none
    CellularAntenna *antenna_ = nullptr;
    bool isNr_ = false;
    double txPower_ = NAN;
    double noiseFigure_ = NAN;
    double cableLoss_ = NAN;
    mutable inet::Coord position_;

  protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

  public:
    virtual ~CellularPhantomRadio();

    /**
     * Adds the radio to the radio medium under the given node id, the one its
     * owner's transmissions name. Called once, by the owner, as soon as it
     * has the id.
     */
    virtual void addToMedium(MacNodeId nodeId);

    /**
     * Puts on the medium the transmission an allocation of the radio's owner
     * stands for (see CellularTransmitter::createPhantomTransmission()):
     * created by the radio's transmitter, added as a phantom transmission.
     */
    virtual void transmit(MacNodeId destId, MacCellId cellId, Direction direction, GHz carrierFrequency, const RbMap& blocks, simtime_t duration,
            const cModule *phantomCell);

    // IRadioEndpoint
    virtual MacNodeId getMacNodeId() override { return nodeId_; }
    virtual const inet::Coord& getCoord() const override;
    virtual double getSpeed() override;
    virtual bool isInsideBuilding() override { return false; }
    virtual double getInsideDistance() override { return 0.0; }
    virtual double getPenetrationLossDeviate() override { return 0.0; }
    virtual TxDirectionType getTxDirection() override;
    virtual double getTxAngle() override;
    virtual double getTxPwr(Direction dir = UNKNOWN_DIRECTION) override { return txPower_; }
    virtual double getAntennaGain() override;
    virtual double getNoiseFigure() override { return noiseFigure_; }
    virtual double getCableLoss() override { return cableLoss_; }
    /** A phantom radio has no receiver: nullptr, as INET's NoiseSource. */
    virtual CellularReceiver *getReceiver() override { return nullptr; }
    /** The medium's channel model of the radio's RAT on the carrier; nullptr for GHz(0.0), as the radio has no primary carrier. */
    virtual ChannelModelBase *getChannelModel(GHz carrierFreq = GHz(0.0)) override;
};

} // namespace simu5g

#endif
