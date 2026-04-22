#include "simu5g/common/GeoUtils.h"
#include "inet/common/geometry/common/Coord.h"

inet::m computeDistance(const GeoCoords& geo1, double alt1, const GeoCoords& geo2, double alt2)
{
    Geocentric earth(Constants::WGS84_a(), Constants::WGS84_f());

    inet::Coord p1, p2;
    earth.Forward(geo1.Latitude(), geo1.Longitude(), alt1, p1.x, p1.y, p1.z);
    earth.Forward(geo2.Latitude(), geo2.Longitude(), alt2, p2.x, p2.y, p2.z);

    return inet::m(p1.distance(p2));
}

double computeUVDistance(const UVCoords& p1, const UVCoords& p2)
{
    return sqrt( (p2.first - p1.first) * (p2.first - p1.first) + (p2.second - p1.second) * (p2.second - p1.second) );
}

double computeUVAngle(const UVCoords& p1, const UVCoords& p2)
{
    double y = p2.second - p1.second;
    double x = p2.first - p1.first;
    return atan2(y,x);
}


double computeAzimuth(const GeoCoords& geo1, double alt1, const GeoCoords& geo2, double alt2)
{
    // see https://tiij.org/issues/issues/3_2/3_2e.html

    double dLong = geo2.Longitude() - geo1.Longitude();
    double dLat = geo2.Latitude() - geo1.Latitude();

    if (dLong == 0 && dLat == 0)
        return 0.0;

    double azimuth = 180 + Math::atand(Math::tand(dLong) / Math::sind(dLat));
    if (dLat >= 0)
        azimuth -= 180;

    return azimuth;
}

double computeElevation(const GeoCoords& geo1, double alt1, const GeoCoords& geo2, double alt2)
{
    // taken from Busquet-Maral, chapter 2.1.6.3
    // not sure this is correct when one endpoint is different from the satellite

    double maxAlt = (alt1 > alt2) ? alt1 : alt2;

    double dLong = geo2.Longitude() - geo1.Longitude();
    double dLat = geo2.Latitude() - geo1.Latitude();

    if (dLong == 0 && dLat == 0)
         return 0.0;

    double cosPhi = Math::cosd(dLong) * Math::cosd(geo1.Latitude()) * Math::cosd(geo2.Latitude()) + Math::sind(geo1.Latitude()) * Math::sind(geo2.Latitude());
    double Re_r = (EARTH_RADIUS / (EARTH_RADIUS + maxAlt));
    double R_r = sqrt(1 + Re_r * Re_r - 2 * Re_r * cosPhi);
    double sinEl = (cosPhi - Re_r) / R_r;
    double elevation = asin(sinEl) * 180 / M_PI;

//    if (alt2 < alt1)
//        elevation = (-1) * elevation;

    return elevation;
}

double computeNadir(double elevation, double alt)
{
    // https://continuouswave.com/forum/viewtopic.php?t=2957
    double r = EARTH_RADIUS + alt;
    double nadirRad = asin(EARTH_RADIUS * Math::sind(elevation+90) / r);
    return inet::math::rad2deg(nadirRad);
}

double computeAlpha(const GeoCoords& dPos, double alt)
{
    double den = alt + EARTH_RADIUS * ( 1 - Math::cosd(dPos.Latitude()) * Math::cosd(dPos.Longitude()));
    return atan( (EARTH_RADIUS * Math::cosd(dPos.Latitude()) * Math::sind(dPos.Longitude())) / den );   // in radians
}

double computeBeta(const GeoCoords& dPos, double alt, double alpha)
{
    double den = alt + EARTH_RADIUS * ( 1 - Math::cosd(dPos.Latitude()) * Math::cosd(dPos.Longitude()));
    return atan( (EARTH_RADIUS * Math::sind(dPos.Latitude()) * cos(alpha) ) / den );  // in radians
}

double computeRelativeAngle(const GeoCoords& dPos1, const GeoCoords& dPos2, double alt)
{
    // see Maral-Bousquet, chapter 9.7.4

    // compute angles with respect to point 1
    double den1 = alt + EARTH_RADIUS * ( 1 - Math::cosd(dPos1.Latitude()) * Math::cosd(dPos1.Longitude()));
    double alpha1 = atan( (EARTH_RADIUS * Math::cosd(dPos1.Latitude()) * Math::sind(dPos1.Longitude())) / den1 );
    double beta1 = atan( (EARTH_RADIUS * Math::sind(dPos1.Latitude()) * cos(alpha1) ) / den1 );

    // compute angles with respect to point 2
    double den2 = alt + EARTH_RADIUS * ( 1 - Math::cosd(dPos2.Latitude()) * Math::cosd(dPos2.Longitude()));
    double alpha2 = atan( (EARTH_RADIUS * Math::cosd(dPos2.Latitude()) * Math::sind(dPos2.Longitude())) / den2 );
    double beta2 = atan( (EARTH_RADIUS * Math::sind(dPos2.Latitude()) * cos(alpha2) ) / den2 );

    // compute the difference
    double dAlpha = fabs(alpha1 - alpha2);
    double dBeta = fabs(beta1 - beta2);

    double cosTheta = cos(dAlpha) * cos(dBeta);
    double theta = inet::math::rad2deg(acos(cosTheta));

    return theta;
}

UVCoords geoCoordsToUvCoords(const GeoCoords& p, const GeoCoords& satCoords, double satAltitude)
{
    GeoCoords dPos(satCoords.Latitude() - p.Latitude(), satCoords.Longitude() - p.Longitude());
    double alpha = computeAlpha(dPos, satAltitude);
    double beta = computeBeta(dPos, satAltitude, alpha);
    double u = sin(beta);
    double v = cos(beta)*sin(alpha);
    return UVCoords(u,v);
}

bool isIntersecting(const GeoCoords& x1, const GeoCoords& x2, const GeoCoords& y1, const GeoCoords& y2)
{
    Geoid geoid(Geoid::DefaultGeoidName());
    Intersect intersect( Geodesic(geoid.EquatorialRadius(), geoid.Flattening()) );
    int segmode;
    intersect.Segment(x1.Latitude(), x1.Longitude(), x2.Latitude(), x2.Longitude(), y1.Latitude(), y1.Longitude(), y2.Latitude(), y2.Longitude(), segmode);
    if (segmode == 0)
        return true;
    return false;
}
