//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_D2DUEMACHELPER_H_
#define _LTE_D2DUEMACHELPER_H_

#include "simu5g/common/LteCommon.h"

namespace inet { class Packet; }

namespace simu5g {

class LteMacUe;
class Binder;
class UserTxParams;
class ID2dMacEnb;

/*
 * Plain helper that holds the D2D-specific state and logic shared by every
 * D2D-capable UE MAC (LTE and NR variants). It is owned by value by the MAC
 * module and only uses the public API of LteMacUe/LteMacBase, so that a MAC
 * module that does NOT derive from LteMacUeD2D can reuse it.
 */
class D2dUeMacHelper
{
  protected:
    // the MAC module owning this helper
    LteMacUe *mac_;

    // reference to the serving eNB's D2D MAC view
    ID2dMacEnb *enb_ = nullptr;

    // RAC handling: a RAC has been requested for a D2D multicast (one-to-many) flow
    bool racD2DMulticastRequested_ = false;

    // Multicast D2D BSR handling: a BSR is pending for a D2D multicast flow
    bool bsrD2DMulticastTriggered_ = false;

    // if true, use the preconfigured TX params for transmission, else use those signaled by the eNB
    bool usePreconfiguredTxParams_ = false;
    UserTxParams *preconfiguredTxParams_ = nullptr;

    // signal for the mode-switch-notification statistic; interned at runtime (see the
    // "Signals" note in D2dUeMacBase.h)
    simsignal_t rcvdD2DModeSwitchNotificationSignal_ = SIMSIGNAL_NULL;

    // build and return new preconfigured user tx params (caller takes ownership)
    UserTxParams *buildPreconfiguredTxParams(Binder *binder);

  public:
    explicit D2dUeMacHelper(LteMacUe *mac)
        : mac_(mac), rcvdD2DModeSwitchNotificationSignal_(cComponent::registerSignal("rcvdD2DModeSwitchNotification")) {}
    ~D2dUeMacHelper();

    // one-time WATCH registration; call from the UE MAC's initialize()
    void initWatches();

    // serving eNB's D2D MAC view
    ID2dMacEnb *getEnb() const { return enb_; }
    void setEnb(ID2dMacEnb *enb) { enb_ = enb; }

    // RAC handling state for D2D multicast
    bool getRacD2DMulticastRequested() const { return racD2DMulticastRequested_; }
    void setRacD2DMulticastRequested(bool v) { racD2DMulticastRequested_ = v; }

    // BSR handling state for D2D multicast
    bool getBsrD2DMulticastTriggered() const { return bsrD2DMulticastTriggered_; }
    void setBsrD2DMulticastTriggered(bool v) { bsrD2DMulticastTriggered_ = v; }

    // preconfigured TX params
    bool getUsePreconfiguredTxParams() const { return usePreconfiguredTxParams_; }
    void setUsePreconfiguredTxParams(bool v) { usePreconfiguredTxParams_ = v; }
    UserTxParams *getPreconfiguredTxParams() const { return preconfiguredTxParams_; }

    // (re)build and store the preconfigured TX params, deleting any previous set
    void rebuildPreconfiguredTxParams(Binder *binder);

    // create a BSR-only MAC PDU for the given buffer occupancy (caller takes ownership)
    inet::Packet *makeBsr(int size);

    // handle a D2D mode switch notification received from the lower layer
    void macHandleD2DModeSwitch(cPacket *pkt);
};

} //namespace

#endif
