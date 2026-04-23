#include "simu5g/mobility/satellite/GeoSatMobility.h"

namespace simu5g {

Define_Module(GeoSatMobility);

void GeoSatMobility::initialize( int stage )
{
    StationaryMobility::initialize( stage );
    if (stage != inet::INITSTAGE_SINGLE_MOBILITY)
        return;

    referenceSystem_ = GeographicReferenceSystemAccess().get();
    ASSERT(referenceSystem_ != nullptr);

    inet::GeoCoord satelliteWgs84(inet::deg(par("initialLatitude")), inet::deg(par("initialLongitude")), inet::m(par("initialAltitude")));
    EV_TRACE << "GeoSatMobility sat_pos_wgs84: (lat(deg) " << satelliteWgs84.latitude
             << ", lon(deg) " << satelliteWgs84.longitude
             << ", alt(m) " << satelliteWgs84.altitude << ")" << std::endl;

    lastPosition = referenceSystem_->omnetFromWgs84(satelliteWgs84);

    EV_TRACE << "GeoSatMobility satellite position: " << lastPosition << std::endl;

    emitMobilityStateChangedSignal();
    refreshDisplay();

    EV_TRACE << "GeoSatMobility updated display string" << std::endl;
}

}
