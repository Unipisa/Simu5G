#include "simu5g/mobility/satellite/GeoSatMobility.h"

using namespace simu5g;

Define_Module(GeoSatMobility);

void GeoSatMobility::initialize( int stage )
{
    StationaryMobility::initialize( stage );
    if (stage == space_veins::INITSTAGE_SPACEVEINS_SATMOBILITY)
    {
        // has to be done after the SOP stage in which the sop_omnet_coord is retrieved from its mobility
        // Get access to SOP
        sop_ = space_veins::SatelliteObservationPointAccess().get();

        // read position parameters
        double lat = par("initialLatitude");
        double lon = par("initialLongitude");
        double alt = par("initialAltitude");
        space_veins::WGS84Coord sat_pos_wgs84 = space_veins::WGS84Coord(lat, lon, alt);

        EV_TRACE << "GeoSatMobility sat_pos_wgs84: " << sat_pos_wgs84 << std::endl;

        // Transform satellite's WGS84 coordinate from geodetic to cartesian representation, proj needs Radians for an unknown reason
        // see https://proj.org/operations/conversions/cart.html
        PJ_COORD toTransfer = proj_coord(sat_pos_wgs84.lon * (PI/180), sat_pos_wgs84.lat * (PI/180), sat_pos_wgs84.alt, 0);
        PJ_COORD geo_cart = proj_trans(wgs84_to_wgs84cartesian_projection, PJ_FWD, toTransfer);
        EV_TRACE << "GeoSatMobility sat_pos_wgs84 cartesian: x: " << geo_cart.xyz.x << ", y: " << geo_cart.xyz.y << ", z: " << geo_cart.xyz.z << std::endl;

        // Geocentric to topocentric, see https://proj.org/operations/conversions/topocentric.html
        PJ_COORD topo_cart = proj_trans(wgs84cartesian_to_topocentric_projection, PJ_FWD, geo_cart);
        EV_TRACE << "GeoSatMobility topo as cartesian coordinates: e: " << topo_cart.enu.e << ", n:" << -topo_cart.enu.n << ", u: " << topo_cart.enu.u << std::endl;

        // Note the minus operator at the northing: The reason is OMNeT++'s coordinate system. The origin is in the upper left corner,
        // the x-axis goes from west to east in the positiv direction and the y-axis goes from north to south in the positiv direction.
        // According to the figure at https://proj.org/operations/conversions/topocentric.html the enu.n-axis needs to be inverted.
        //
        // Further, the position of the SOP is added such that the satellite position is relative to OMNeT++'s origin.
        auto sop_omnet_coord = sop_->get_sop_omnet_coord();
        inet::Coord satellitePosition(topo_cart.enu.e + sop_omnet_coord.x, -topo_cart.enu.n + sop_omnet_coord.y, topo_cart.enu.u + sop_omnet_coord.z);
        EV_TRACE << "GeoSatMobility satellite position: " << satellitePosition << std::endl;

        lastPosition = satellitePosition;
    }
}


void GeoSatMobility::initializePosition()
{
//    checkPosition();
//    emitMobilityStateChangedSignal();
}
