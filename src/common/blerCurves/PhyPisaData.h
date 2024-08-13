//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _PHYPISADATA_H_
#define _PHYPISADATA_H_

#include <string.h>
#include <vector>
#include <iostream>

#include "common/blerCurves/BLERvsSINR_15CQI_TU.h"

namespace simu5g {

class PhyPisaData
{
    double lambdaTable_[10000][3];
    double blerCurves_[3][15][49];
    std::vector<double> channel_;

    int blerShift_ = 0;

  public:
    PhyPisaData();
    virtual ~PhyPisaData();

    double getLambda(int i, int j) { return lambdaTable_[i][j]; }
    int nTxMode() { return 3; }
    int nMcs() { return 15; }

    int maxChannel() { return 10000; }
    int maxChannel2() { return 1000; }

    // getBler gets the following parameters: (txMode , CQI, SINR)
    //double getBler(int i, int j, int k){if (j==0) return 1; else return blerCurves_[i][j][k-1+blerShift_];}
    double getBler(int i, int j, int k) { return GetBLER_TU(k + blerShift_, j); };
    int minSnr() { return -14 - blerShift_; }//SINR_15_CQI_TU [0] [0];}
    int maxSnr() { return 40 - blerShift_; }//SINR_15_CQI_TU [14] [15];}

    void setBlerShift(int shift) { blerShift_ = shift; }
    double getChannel(unsigned int i);
};

} //namespace

#endif

