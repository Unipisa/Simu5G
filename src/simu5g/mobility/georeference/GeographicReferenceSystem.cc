#include "simu5g/mobility/georeference/GeographicReferenceSystem.h"

#include <cmath>
#include <sstream>

namespace simu5g {

using namespace omnetpp;

Define_Module(GeographicReferenceSystem);

namespace {
constexpr double DEG_TO_RAD = M_PI / 180.0;
}

GeographicReferenceSystem::~GeographicReferenceSystem()
{
    if (wgs84CartesianToTopocentricProjection_ != nullptr)
        proj_destroy(wgs84CartesianToTopocentricProjection_);
    if (wgs84ToWgs84CartesianProjection_ != nullptr)
        proj_destroy(wgs84ToWgs84CartesianProjection_);
    if (pjCtx_ != nullptr)
        proj_context_destroy(pjCtx_);
}

void GeographicReferenceSystem::initialize(int stage)
{
    if (stage != inet::INITSTAGE_LOCAL)
        return;

    referenceOmnetCoord_ = inet::Coord(par("x"), par("y"), par("z"));
    referenceWgs84_ = inet::GeoCoord(inet::deg(par("referenceLatitude")), inet::deg(par("referenceLongitude")), inet::m(par("referenceAltitude")));

    pjCtx_ = proj_context_create();
    wgs84ToWgs84CartesianProjection_ = proj_create(pjCtx_, "+proj=pipeline +step proj=cart +ellps=WGS84");

    ASSERT(wgs84ToWgs84CartesianProjection_ != nullptr);

    PJ_COORD referenceWgs84ProjRad = proj_coord(referenceWgs84_.longitude.get() * DEG_TO_RAD, referenceWgs84_.latitude.get() * DEG_TO_RAD, referenceWgs84_.altitude.get(), 0);
    referenceWgs84ProjCart_ = proj_trans(wgs84ToWgs84CartesianProjection_, PJ_FWD, referenceWgs84ProjRad);

    std::stringstream ssProj;
    ssProj << "+proj=topocentric +ellps=WGS84 +X_0=" << referenceWgs84ProjCart_.xyz.x
           << " +Y_0=" << referenceWgs84ProjCart_.xyz.y
           << " +Z_0=" << referenceWgs84ProjCart_.xyz.z;
    wgs84CartesianToTopocentricProjection_ = proj_create(pjCtx_, ssProj.str().c_str());
    ASSERT(wgs84CartesianToTopocentricProjection_ != nullptr);
}

inet::Coord GeographicReferenceSystem::omnetFromWgs84(const inet::GeoCoord& wgs84Coord) const
{
    PJ_COORD toTransfer = proj_coord(wgs84Coord.longitude.get() * DEG_TO_RAD, wgs84Coord.latitude.get() * DEG_TO_RAD, wgs84Coord.altitude.get(), 0);
    PJ_COORD geoCart = proj_trans(wgs84ToWgs84CartesianProjection_, PJ_FWD, toTransfer);
    PJ_COORD topoCart = proj_trans(wgs84CartesianToTopocentricProjection_, PJ_FWD, geoCart);

    return inet::Coord(
            topoCart.enu.e + referenceOmnetCoord_.x,
            -topoCart.enu.n + referenceOmnetCoord_.y,
            topoCart.enu.u + referenceOmnetCoord_.z);
}

} // namespace simu5g
