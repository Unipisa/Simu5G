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

#ifndef __GEOUTILS_H_
#define __GEOUTILS_H_

// Temporarily hide INET's NaN macro while parsing GeographicLib headers
#ifdef NaN
  #pragma push_macro("NaN")
  #undef NaN
  #define GEOUTILS_RESTORE_NAN
#endif

#include "GeographicLib/GeoCoords.hpp"
#include "GeographicLib/Geoid.hpp"
#include "GeographicLib/Geodesic.hpp"
#include "GeographicLib/Geocentric.hpp"
#include "GeographicLib/Intersect.hpp"

// Restore NaN macro if it existed
#ifdef GEOUTILS_RESTORE_NAN
  #pragma pop_macro("NaN")
  #undef GEOUTILS_RESTORE_NAN
#endif

#include "inet/common/Units_m.h"

using namespace GeographicLib;

const double EARTH_RADIUS = 6371000;   // 6371km (average radius of the geoid)

typedef std::pair<double, double> UVCoords;

// returns the geocentric distance between two geo points
inet::m computeDistance(const GeoCoords& geo1, double alt1, const GeoCoords& geo2, double alt2);
// returns the distance between two u-v coords
double computeUVDistance(const UVCoords& p1, const UVCoords& p2);
// returns the angle (in radians) between two u-v coords
double computeUVAngle(const UVCoords& p1, const UVCoords& p2);

// returns the relative azimuth between the two specified geographical coordinates
double computeAzimuth(const GeoCoords& geo1, double alt1, const GeoCoords& geo2, double alt2);

// returns the relative elevation between the two specified geographical coordinates
double computeElevation(const GeoCoords& geo1, double alt1, const GeoCoords& geo2, double alt2);

// returns the nadir angle
double computeNadir(double elevation, double alt);

double computeAlpha(const GeoCoords& dPos, double alt);
double computeBeta(const GeoCoords& dPos, double alt, double alpha);
double computeRelativeAngle(const GeoCoords& dPos1, const GeoCoords& dPos2, double alt);

// do conversion from (lat,lon) coords to (u,v) coords
UVCoords geoCoordsToUvCoords(const GeoCoords& p, const GeoCoords& satCoords, double satAltitude);

// returns true if segments x1,x2 and y1,y2 intersect each other
bool isIntersecting(const GeoCoords& x1, const GeoCoords& x2, const GeoCoords& y1, const GeoCoords& y2);

#endif
