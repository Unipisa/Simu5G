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

#ifndef _BACKGROUNDCELLAMC_H_
#define _BACKGROUNDCELLAMC_H_

#include <omnetpp.h>

#include "common/binder/Binder.h"
#include "stack/mac/amc/LteMcs.h"

namespace simu5g {

using namespace omnetpp;

class BackgroundCellAmc
{
  protected:

    // reference to the binder
    opp_component_ptr<Binder> binder_;

    McsTable dlMcsTable_;
    McsTable ulMcsTable_;
    McsTable d2dMcsTable_;
    double mcsScaleDl_;
    double mcsScaleUl_;
    double mcsScaleD2D_;

    void calculateMcsScale();

  public:
    BackgroundCellAmc(Binder *binder);
    virtual ~BackgroundCellAmc() {}

    virtual unsigned int computeBitsPerRbBackground(Cqi cqi, const Direction dir, double carrierFrequency);

    // utilities - do not involve pilot invocation
    unsigned int getItbsPerCqi(Cqi cqi, const Direction dir);

    /*
     * Access the correct itbs2tbs conversion table given cqi and number of layers
     */
    const unsigned int *readTbsVect(Cqi cqi, unsigned int layers, Direction dir);

    /*
     * given <cqi> and <layers> returns bytes allocable in <blocks>
     */
    unsigned int blockGain(Cqi cqi, unsigned int layers, unsigned int blocks, Direction dir);

    /*
     * given <cqi> and <layers> returns blocks capable of carrying  <bytes>
     */
    unsigned int bytesGain(Cqi cqi, unsigned int layers, unsigned int bytes, Direction dir);
};

} //namespace

#endif

