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

  protected:
    virtual void initialize() override;

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
};

} // namespace simu5g

#endif
