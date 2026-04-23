package simu5g.mobility.satellite;

import inet.mobility.static.StationaryMobility;

//
// Mobility module for GEO satellites
//
simple GeoSatMobility extends StationaryMobility
{
    parameters:
        @class(simu5g::GeoSatMobility);
        initialLatitude = default(0deg);    // initial latitude (in degrees)
        initialLongitude = default(0deg);   // initial longitude (in degrees)
        initialAltitude = default(35786km); // initial altitude (in meters)
}
