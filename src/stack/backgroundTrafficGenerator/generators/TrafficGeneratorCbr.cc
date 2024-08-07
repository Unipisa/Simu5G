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

#include "stack/backgroundTrafficGenerator/generators/TrafficGeneratorCbr.h"

namespace simu5g {

Define_Module(TrafficGeneratorCbr);

void TrafficGeneratorCbr::initialize(int stage)
{
    TrafficGeneratorBase::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        // read parameters
        size_[DL] = par("packetSizeDl");
        size_[UL] = par("packetSizeUl");

        period_[DL] = par("periodDl");
        period_[UL] = par("periodUl");
    }
}

unsigned int TrafficGeneratorCbr::generateTraffic(Direction dir)
{
    bufferedBytes_[dir] += (size_[dir] + headerLen_);
    return size_[dir] + headerLen_;
}

simtime_t TrafficGeneratorCbr::getNextGenerationTime(Direction dir)
{
    return (simtime_t)period_[dir];
}

double TrafficGeneratorCbr::getAvgLoad(Direction dir)
{
    double avgLoad = (double)(size_[dir] + headerLen_) / period_[dir];
    return avgLoad;
}

} //namespace

