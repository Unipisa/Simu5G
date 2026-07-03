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

#ifndef STACK_PHY_ERRORMODEL_ERRORMODEL_H_
#define STACK_PHY_ERRORMODEL_ERRORMODEL_H_

#include <vector>

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/LteControlInfo.h"
#include "simu5g/stack/phy/packet/LteAirFrame.h"

namespace simu5g {

using namespace omnetpp;

class Binder;
class LteChannelModel;

class ErrorModel : public cSimpleModule
{
  protected:
    struct ReceptionParams
    {
        unsigned char codeword = 0;
        int numCodewords = 0;
        Cqi cqi = 0;
        MacNodeId nodeId = NODEID_NONE;
        Direction direction = UNKNOWN_DIRECTION;
        unsigned char transmissionAttempt = 0;
        TxMode txMode = SINGLE_ANTENNA_PORT0;
        unsigned int txModeIndex = 0;
    };

    inet::ModuleRefByPar<Binder> binder_;
    double harqReduction_ = NAN;

    static simsignal_t rcvdSinrDlSignal_;
    static simsignal_t rcvdSinrUlSignal_;
    static simsignal_t rcvdSinrD2DSignal_;

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    ReceptionParams extractReceptionParams(UserControlInfo *lteInfo) const;
    double getBler(unsigned int txModeIndex, Cqi cqi, int snr) const;
    bool drawReceptionResult(double packetErrorRate, const ReceptionParams& params) const;
    void emitSinrStatistics(LteChannelModel *channelModel, UserControlInfo *lteInfo, const ReceptionParams& params, double sumSnr, int usedRBs);

    virtual double computePacketErrorRate(LteAirFrame *frame, UserControlInfo *lteInfo, const std::vector<double>& snrVector, LteChannelModel *channelModel, const ReceptionParams& params, double& sumSnr, int& usedRBs) const;

  public:
    virtual bool isReceptionSuccessful(LteAirFrame *frame, UserControlInfo *lteInfo, const std::vector<double>& snrVector, LteChannelModel *channelModel);
};

} //namespace

#endif
