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

    space_veins::WGS84Coord satelliteWgs84(par("initialLatitude"), par("initialLongitude"), par("initialAltitude"));
    EV_TRACE << "GeoSatMobility sat_pos_wgs84: " << satelliteWgs84 << std::endl;

    lastPosition = referenceSystem_->omnetFromWgs84(satelliteWgs84);

    EV_TRACE << "GeoSatMobility satellite position: " << lastPosition << std::endl;

    emitMobilityStateChangedSignal();
    refreshDisplay();

    EV_TRACE << "GeoSatMobility updated display string" << std::endl;
}

}
