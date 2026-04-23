#ifndef __SIMU5G_GEOSATMOBILITY_H_
#define __SIMU5G_GEOSATMOBILITY_H_

#include <omnetpp.h>

#include "inet/mobility/static/StationaryMobility.h"
#include "inet/common/INETDefs.h"
#include "inet/common/InitStages.h"

#include "simu5g/mobility/georeference/GeographicReferenceSystem.h"

using namespace omnetpp;
using namespace inet;

namespace simu5g {


class GeoSatMobility : public StationaryMobility
{

protected:

    GeographicReferenceSystem *referenceSystem_ = nullptr;

public:

    virtual void initialize  ( int stage ) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
};

}

#endif
