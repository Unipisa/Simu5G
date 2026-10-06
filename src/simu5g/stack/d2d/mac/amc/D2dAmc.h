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

#ifndef STACK_D2D_MAC_AMC_D2DAMC_H_
#define STACK_D2D_MAC_AMC_D2DAMC_H_

#include <cstring>

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/stack/mac/amc/LteAmc.h"
#include "simu5g/stack/d2d/binder/D2dBinder.h"
#include "simu5g/stack/d2d/mac/ID2dAmc.h"
#include "simu5g/stack/d2d/mac/amc/AmcPilotD2D.h"
#include "simu5g/stack/d2d/mac/amc/D2dAmcHelper.h"
#include "simu5g/common/InitStages.h"

namespace simu5g {

/**
 * CRTP mixin that adds device-to-device (D2D) support to an AMC.
 *
 * The core AMCs (LteAmc and its NR subclass NrAmc) carry no D2D code; this
 * mixin layers the D2D feedback and transmission-parameter machinery on top of
 * either of them. The D2D state and heavy logic live in the shared
 * D2dAmcHelper; the mixin only overrides the base *ForDirection routing seams
 * and the ID2dAmc surface to delegate to it.
 *
 * The concrete Define_Module'd AMCs are:
 *   LteAmcD2D = D2dAmc<LteAmc>
 *   NrAmcD2D  = D2dAmc<NrAmc> (+ the NR-specific getNrMcsTableForDirection
 *               override, see NrAmcD2D.h)
 */
template<class Base>
class D2dAmc : public Base, public ID2dAmc, public cListener
{
  protected:
    // holds the D2D-specific AMC state and logic
    D2dAmcHelper d2dHelper_;

    // holder of the global D2D state
    inet::ModuleRefByPar<D2dBinder> d2dBinder_;

    void initialize(int stage) override
    {
        Base::initialize(stage);
        if (stage == inet::INITSTAGE_LOCAL)
            d2dBinder_.reference(this, "d2dBinderModule", true);
        else if (stage == INITSTAGE_SIMU5G_BINDER_ACCESS) {
            d2dHelper_.initD2D(d2dBinder_.get());
            // A departing UE outlives itself in the AMCs of cells it never attached to, as a
            // D2D feedback peer, which detachUser() at its own serving cell cannot reach. The
            // Binder's notification reaches every D2D AMC in the network, so it does.
            this->getBinder()->subscribe(Binder::nodeUnregisteredSignal_, this);
        }
    }

    void receiveSignal(cComponent *source, simsignal_t signalID, long nodeId, cObject *details) override
    {
        ASSERT(signalID == Binder::nodeUnregisteredSignal_);
        d2dHelper_.purgeDepartedNode(MacNodeId(nodeId));
    }

    // the "D2D" amcMode selects the D2D AMC pilot; other modes fall back to the base
    AmcPilot *createAmcPilot(const char *amcMode) override
    {
        if (strcmp(amcMode, "D2D") == 0) {
            EV << "Creating Amc pilot " << amcMode << endl;
            return new AmcPilotD2D(this->binder_, this);
        }
        return Base::createAmcPilot(amcMode);
    }

    // D2D branch of the base routing seams
    void printTxParamsForDirection(Direction dir, GHz carrierFrequency) override
    {
        if (dir == D2D)
            d2dHelper_.printTxParamsD2D(carrierFrequency);
        else
            Base::printTxParamsForDirection(dir, carrierFrequency);
    }

    bool existTxParamsForDirection(MacNodeId id, Direction dir, GHz carrierFrequency) override
    {
        if (dir == D2D)
            return d2dHelper_.existTxParamsD2D(id, carrierFrequency);
        return Base::existTxParamsForDirection(id, dir, carrierFrequency);
    }

    const UserTxParams& setTxParamsForDirection(MacNodeId id, Direction dir, UserTxParams& info, GHz carrierFrequency) override
    {
        if (dir == D2D)
            return d2dHelper_.setTxParamsD2D(id, info, carrierFrequency);
        return Base::setTxParamsForDirection(id, dir, info, carrierFrequency);
    }

    const UserTxParams& getTxParamsForDirection(MacNodeId id, Direction dir, GHz carrierFrequency) override
    {
        if (dir == D2D)
            return d2dHelper_.getTxParamsD2D(id, carrierFrequency);
        return Base::getTxParamsForDirection(id, dir, carrierFrequency);
    }

    McsTable *getMcsTableForDirection(Direction dir) override
    {
        if (dir == D2D || dir == D2D_MULTI)
            return &this->ulMcsTable_;
        return Base::getMcsTableForDirection(dir);
    }

    void rescaleMcsForDirection(double rePerRb, Direction dir) override
    {
        if (dir == D2D)
            d2dHelper_.rescaleD2D(rePerRb);
        else
            Base::rescaleMcsForDirection(rePerRb, dir);
    }

    void detachUserForDirection(MacNodeId nodeId, Direction dir) override
    {
        if (dir == D2D)
            d2dHelper_.detachUserD2D(nodeId);
        else
            Base::detachUserForDirection(nodeId, dir);
    }

    void attachUserForDirection(MacNodeId nodeId, Direction dir) override
    {
        if (dir == D2D)
            d2dHelper_.attachUserD2D(nodeId);
        else
            Base::attachUserForDirection(nodeId, dir);
    }

    void testUeForDirection(MacNodeId nodeId, Direction dir) override
    {
        if (dir == D2D)
            d2dHelper_.testUeD2D(nodeId);
        else
            Base::testUeForDirection(nodeId, dir);
    }

  public:
    D2dAmc() : d2dHelper_(this) {}

    // ID2dAmc
    void pushFeedbackD2D(MacNodeId id, LteFeedback fb, MacNodeId peerId, GHz carrierFrequency) override
    {
        d2dHelper_.pushFeedbackD2D(id, fb, peerId, carrierFrequency);
    }

    const LteSummaryFeedback& getFeedbackD2D(MacNodeId id, Remote antenna, TxMode txMode, MacNodeId peerId, GHz carrierFrequency) override
    {
        return d2dHelper_.getFeedbackD2D(id, antenna, txMode, peerId, carrierFrequency);
    }
};

} //namespace

#endif
