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

#ifndef _NRPHYUE_H_
#define _NRPHYUE_H_

#include <map>

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/stack/phy/LtePhyUeD2D.h"
#include "simu5g/stack/phy/antennamodel/IAntennaModel.h"

namespace simu5g {

class NrPhyUe : public LtePhyUeD2D
{
  protected:
    std::map<GHz, opp_component_ptr<LteChannelModel>> ntnChannelModel_;
    inet::ModuleRefByPar<LteChannelModel> primaryNtnChannelModel_;
    inet::ModuleRefByPar<IAntennaModel> ntnAntennaModel_;

    void initialize(int stage) override;
    void initializeChannelModels();
    void handleAirFrame(cMessage *msg) override;
    void sendUnicast(LteAirFrame *airFrame) override;
    virtual bool sendUnicastViaNtn(LteAirFrame *airFrame);

  public:
    LteChannelModel *getReceptionChannelModel(const UserControlInfo *lteInfo) override;
};

} //namespace

#endif /* _NRPHYUE_H_ */
