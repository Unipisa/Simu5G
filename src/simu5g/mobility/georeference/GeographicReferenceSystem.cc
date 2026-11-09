#include "simu5g/mobility/georeference/GeographicReferenceSystem.h"

#include <cmath>

namespace simu5g {

using namespace omnetpp;

Define_Module(GeographicReferenceSystem);

GeographicReferenceSystem::GeographicReferenceSystem() :
    earth_(GeographicLib::Constants::WGS84_a(), GeographicLib::Constants::WGS84_f()),
    localFrame_(earth_)
{
}

void GeographicReferenceSystem::initialize(int stage)
{
    if (stage != inet::INITSTAGE_LOCAL)
        return;

    referenceOmnetCoord_ = inet::Coord(par("x"), par("y"), par("z"));
    referenceWgs84_ = inet::GeoCoord(inet::deg(par("referenceLatitude")), inet::deg(par("referenceLongitude")), inet::m(par("referenceAltitude")));
    earth_.Forward(referenceWgs84_.latitude.get(), referenceWgs84_.longitude.get(), referenceWgs84_.altitude.get(),
            referenceEcefCoord_.x, referenceEcefCoord_.y, referenceEcefCoord_.z);
    localFrame_.Reset(referenceWgs84_.latitude.get(), referenceWgs84_.longitude.get(), referenceWgs84_.altitude.get());
}

inet::Coord GeographicReferenceSystem::omnetFromWgs84(const inet::GeoCoord& wgs84Coord) const
{
    double east = 0;
    double north = 0;
    double up = 0;
    localFrame_.Forward(wgs84Coord.latitude.get(), wgs84Coord.longitude.get(), wgs84Coord.altitude.get(), east, north, up);

    return inet::Coord(
            east + referenceOmnetCoord_.x,
            -north + referenceOmnetCoord_.y,
            up + referenceOmnetCoord_.z);
}

inet::GeoCoord GeographicReferenceSystem::wgs84FromOmnet(const inet::Coord& omnetCoord) const
{
    double east = omnetCoord.x - referenceOmnetCoord_.x;
    double north = -(omnetCoord.y - referenceOmnetCoord_.y);
    double up = omnetCoord.z - referenceOmnetCoord_.z;
    double latitude = 0;
    double longitude = 0;
    double altitude = 0;
    localFrame_.Reverse(east, north, up, latitude, longitude, altitude);
    return inet::GeoCoord(inet::deg(latitude), inet::deg(longitude), inet::m(altitude));
}

inet::Coord GeographicReferenceSystem::omnetVectorFromEcef(const inet::Coord& ecefVector) const
{
    // East/north/up unit vectors at the (geodetic) reference point, the same basis localFrame_
    // uses, mapped onto the OMNeT++ convention of omnetFromWgs84(): x = east, y = -north, z = up.
    double lat = inet::rad(referenceWgs84_.latitude).get();
    double lon = inet::rad(referenceWgs84_.longitude).get();
    double sinLat = std::sin(lat), cosLat = std::cos(lat);
    double sinLon = std::sin(lon), cosLon = std::cos(lon);

    double east = -sinLon * ecefVector.x + cosLon * ecefVector.y;
    double north = -sinLat * cosLon * ecefVector.x - sinLat * sinLon * ecefVector.y + cosLat * ecefVector.z;
    double up = cosLat * cosLon * ecefVector.x + cosLat * sinLon * ecefVector.y + sinLat * ecefVector.z;
    return inet::Coord(east, -north, up);
}

} // namespace simu5g
