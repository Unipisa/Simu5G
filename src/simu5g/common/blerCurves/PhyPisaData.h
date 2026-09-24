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

#ifndef _PHYPISADATA_H_
#define _PHYPISADATA_H_

#include <vector>
#include <iostream>

#include "simu5g/common/blerCurves/BLERvsSINR_15CQI_TU.h"

namespace simu5g {

class PhyPisaData
{
    std::vector<double> channel_;

    int blerShift_ = 0;

  public:
    PhyPisaData();
    virtual ~PhyPisaData();

    int nCqi() { return 15; }

    // BLER of the given CQI (1-15, per 3GPP) at the given SINR (dB), read from the
    // single TU curve set of BLERvsSINR_15CQI_TU.h, whatever the transmission mode
    double getBler(int cqi, int sinr) { return GetBLER_TU(sinr + blerShift_, cqi); };
    int minSnr() { return -14 - blerShift_; }//SINR_15_CQI_TU [0] [0];}
    int maxSnr() { return 40 - blerShift_; }//SINR_15_CQI_TU [14] [15];}

    void setBlerShift(int shift) { blerShift_ = shift; }
    double getChannel(unsigned int i);
};

} //namespace

#endif

