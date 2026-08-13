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

#include <GeographicLib/Constants.hpp>
#include "GeographicLib/Geocentric.hpp"

// Restore NaN macro if it existed
#ifdef GEOUTILS_RESTORE_NAN
  #pragma pop_macro("NaN")
  #undef GEOUTILS_RESTORE_NAN
#endif

#include "inet/common/geometry/common/Coord.h"
#include "inet/common/geometry/common/GeographicCoordinateSystem.h"

using namespace GeographicLib;

// converts a WGS84 point into an ECEF Cartesian point
inet::Coord ecefFromWgs84(const inet::GeoCoord& wgs84Coord);

// returns the elevation angle of the target endpoint as seen from the observer point.
// The computation follows the usual geometric definition in an Earth-centered frame:
// build the line-of-sight vector from the observer endpoint to the target endpoint in ECEF,
// project it onto the local zenith (Up) direction at the observer point, and derive the
// elevation from that projection.
//
//                         target endpoint
//                              *
//                             /
//                            /
//                           /   LOS
//                          /
//                         /  E
//                        /
//                       *
//                       +-------------------- local horizontal
//                       |
//                       |
//                       | Up
//                       |
//                 observer endpoint
//
// See ESA Navipedia, "Transformations between ECEF and ENU coordinates", especially the
// "Elevation and azimuth computation" section
// (https://gssc.esa.int/navipedia/index.php/Transformations_between_ECEF_and_ENU_coordinates)
double computeElevationFromEcefEndpoints(const inet::GeoCoord& observerWgs84, const inet::Coord& observerEcef, const inet::Coord& targetEcef);

// returns the slant range, in metres, from a point on the Earth's surface to a satellite at the
// given altitude seen at the given elevation angle (in degrees). It is the inverse of
// computeElevationFromEcefEndpoints() for the special case of a spherical Earth, and it is
// monotonically decreasing in the elevation: the *lowest* elevation a cell is willing to use
// therefore yields the *longest* link that cell can have.
//
// The sphere is taken at the WGS84 equatorial radius, so that this file has a single Earth model
// and one source of truth for it -- the same GeographicLib constant ecefFromWgs84() converts
// against. The equatorial radius specifically, rather than a mean one, because the slant range
// grows monotonically with the Earth radius: it is therefore the value that *maximises* the range
// and so keeps the result an upper bound at every latitude, which is what the callers need.
//
// This is how TR 38.821 clause 6.1.1 derives its reference round-trip delays, and evaluating this
// formula at a minimum elevation of 10 degrees reproduces them: 40586.1km for a GEO satellite and
// 1932.3km for a 600km LEO satellite, which over four hops (service and feeder link, each
// traversed twice) give 541.52ms and 25.78ms against the 541.46ms and 25.77ms of clause 7.2. The
// residual 0.01% is the Earth model: TR 38.821 works on a 6371km sphere, which fed into this same
// formula reproduces its figures to the digit. Every timer derived from these delays rounds to a
// TS 38.331 enumeration and is identical under either radius.
//
//                          satellite
//                              *
//                             /|
//                            / | altitude
//                slantRange /  |
//                          /   |
//                         / E  |
//                        *-----+------- local horizontal
//                   observer
//
// Derived by the law of cosines in the triangle observer-geocentre-satellite, whose sides are the
// Earth radius, the Earth radius + altitude and the slant range, and whose angle at the observer
// is 90 degrees + elevation.
double computeSlantRangeAtElevation(double altitude, double elevation);

#endif
