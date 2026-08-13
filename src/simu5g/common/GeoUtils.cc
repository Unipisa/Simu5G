#include "simu5g/common/GeoUtils.h"
#include "inet/common/geometry/common/Coord.h"

inet::Coord ecefFromWgs84(const inet::GeoCoord& wgs84Coord)
{
    Geocentric earth(Constants::WGS84_a(), Constants::WGS84_f());

    inet::Coord ecefCoord;
    earth.Forward(wgs84Coord.latitude.get(), wgs84Coord.longitude.get(), wgs84Coord.altitude.get(), ecefCoord.x, ecefCoord.y, ecefCoord.z);
    return ecefCoord;
}

double computeElevationFromEcefEndpoints(const inet::GeoCoord& observerWgs84, const inet::Coord& observerEcef, const inet::Coord& targetEcef)
{
    inet::Coord losVector = targetEcef - observerEcef;
    double losNorm = losVector.length();
    if (losNorm == 0.0)
        return 90.0;

    double latitudeRad = observerWgs84.latitude.get() * M_PI / 180.0;
    double longitudeRad = observerWgs84.longitude.get() * M_PI / 180.0;
    inet::Coord upVector(
        std::cos(latitudeRad) * std::cos(longitudeRad),
        std::cos(latitudeRad) * std::sin(longitudeRad),
        std::sin(latitudeRad));

    double projection = (losVector.x * upVector.x + losVector.y * upVector.y + losVector.z * upVector.z) / losNorm;
    projection = std::max(-1.0, std::min(1.0, projection));
    return std::asin(projection) * 180.0 / M_PI;
}


double computeSlantRangeAtElevation(double altitude, double elevation)
{
    // Law of cosines on the observer-geocentre-satellite triangle, solved for the observer-to-
    // satellite side. With the angle at the observer equal to 90+E, cos(90+E) = -sin(E), so the
    // quadratic in the slant range d is
    //
    //     d^2 + 2*Re*sin(E)*d - (2*Re*altitude + altitude^2) = 0
    //
    // whose positive root is the expression below. Negative elevations are legal and give a
    // longer range, as they must: the satellite is then below the local horizon.
    double earthRadius = Constants::WGS84_a();
    double sinElevation = std::sin(inet::math::deg2rad(elevation));
    double reSinElevation = earthRadius * sinElevation;
    return std::sqrt(reSinElevation * reSinElevation + altitude * altitude + 2 * earthRadius * altitude) - reSinElevation;
}
