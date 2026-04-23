#ifndef __SIMU5G_GEOSATMOBILITY_H_
#define __SIMU5G_GEOSATMOBILITY_H_

#include <omnetpp.h>
#include <proj.h>

#include "inet/mobility/static/StationaryMobility.h"
#include "inet/common/INETDefs.h"

#include "space_veins/common/InitStages.h"
#include "space_veins/modules/SatelliteObservationPoint/SatelliteObservationPoint.h"

using namespace omnetpp;
using namespace inet;

namespace simu5g {


class GeoSatMobility : public StationaryMobility
{

protected:

    // SOP pointer
    space_veins::SatelliteObservationPoint* sop_;

    /* PROJ objects */
    PJ_CONTEXT* pj_ctx;
    PJ* wgs84_to_wgs84cartesian_projection;
    PJ* wgs84cartesian_to_topocentric_projection;

public:

    virtual void initialize  ( int stage ) override;
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
};

}

#endif
