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

#ifndef STACK_PHY_RADIO_CELLULARRECEIVER_H_
#define STACK_PHY_RADIO_CELLULARRECEIVER_H_

#include <omnetpp.h>

#include "simu5g/common/LteCommon.h"

namespace simu5g {

using namespace omnetpp;

class BlerCurveErrorModel;

/**
 * See the NED documentation of CellularReceiver.
 */
class CellularReceiver : public cSimpleModule
{
  protected:
    double noiseFigure_ = NAN;
    double cableLoss_ = NAN;

    static simsignal_t rcvdSinrDlSignal_;
    static simsignal_t rcvdSinrUlSignal_;
    static simsignal_t measuredSinrDlSignal_;
    static simsignal_t measuredSinrUlSignal_;

    /** The per-carrier signals of one carrier, when the radio serves more than one. */
    struct CarrierSignals
    {
        simsignal_t rcvdSinrDl;
        simsignal_t rcvdSinrUl;
        simsignal_t measuredSinrDl;
        simsignal_t measuredSinrUl;
    };
    std::map<GHz, CarrierSignals> carrierSignals_;

  protected:
    virtual void initialize() override;

    /** Registers the per-carrier signal of a statistic and attaches its recorders. */
    simsignal_t addCarrierStatistic(const char *statisticName, GHz carrierFrequency);

    /** The carrier's signals, or nullptr if the radio serves only one carrier. */
    const CarrierSignals *findCarrierSignals(GHz carrierFrequency) const;

    /** Emits on a per-carrier signal, once the warm-up period is over. */
    void emitOnCarrier(simsignal_t signal, double value);

  public:
    /** The error model the reception decisions are drawn against. */
    BlerCurveErrorModel *getErrorModel() const;

    /**
     * Decides a reception with the given packet error rate by one uniform
     * draw from this module's RNG: received if the draw exceeds the rate.
     */
    virtual bool decide(double packetErrorRate);

    /** Noise figure in dB. */
    double getNoiseFigure() const { return noiseFigure_; }
    /** Cable loss in dB. */
    double getCableLoss() const { return cableLoss_; }

    /**
     * The carriers the radio serves. With more than one, every SINR statistic
     * is also recorded per carrier. Called once, during initialization.
     */
    void setCarrierFrequencies(const std::vector<GHz>& carrierFrequencies);

    /**
     * Records the mean SINR over the resource blocks of a decoded frame, on
     * rcvdSinrDl or rcvdSinrUl (dir is DL or UL), in the aggregate statistic
     * and in the carrier's own.
     */
    void emitRcvdSinr(Direction dir, GHz carrierFrequency, double sinr);

    /**
     * Records the mean SINR of a feedback computation, on measuredSinrDl or
     * measuredSinrUl (dir is DL or UL), in the aggregate statistic and in the
     * carrier's own.
     */
    void emitMeasuredSinr(Direction dir, GHz carrierFrequency, double sinr);
};

} // namespace simu5g

#endif
