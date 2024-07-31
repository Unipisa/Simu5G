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

#ifndef BACKGROUNDTRAFFICMANAGER_H_
#define BACKGROUNDTRAFFICMANAGER_H_

#include <inet/common/ModuleRefByPar.h>

#include "common/LteCommon.h"
#include "common/blerCurves/PhyPisaData.h"
#include "stack/backgroundTrafficGenerator/BackgroundTrafficManagerBase.h"
#include "stack/backgroundTrafficGenerator/generators/TrafficGeneratorBase.h"
#include "stack/mac/layer/LteMacEnb.h"
#include "stack/phy/layer/LtePhyEnb.h"

namespace simu5g {

using namespace omnetpp;

class TrafficGeneratorBase;
class LteMacEnb;
class LteChannelModel;


//
// BackgroundTrafficManager
//
class BackgroundTrafficManager : public BackgroundTrafficManagerBase
{
};

} //namespace

#endif
