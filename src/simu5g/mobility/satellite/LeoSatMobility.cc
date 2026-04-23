#include "simu5g/mobility/satellite/LeoSatMobility.h"

#include <cmath>
#include <utility>
#include <vector>

#include "simu5g/mobility/satellite/TEME2ITRF.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(LeoSatMobility);

namespace {
const GeographicLib::Geocentric WGS84_EARTH(
        GeographicLib::Constants::WGS84_a(),
        GeographicLib::Constants::WGS84_f());
}

void LeoSatMobility::preInitialize(TLE tle, std::string wallClockSimStartTimeUtc)
{
    tle_ = std::move(tle);
    wallClockSimStartTimeUtc_ = std::move(wallClockSimStartTimeUtc);
    isPreInitialized_ = true;
}

void LeoSatMobility::initialize(int stage)
{
    MovingMobilityBase::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        if (!isPreInitialized_) {
            tle_ = TLE(par("satelliteName").stdstringValue(), par("tleLine1").stdstringValue(), par("tleLine2").stdstringValue());
            wallClockSimStartTimeUtc_ = par("wallClockSimStartTimeUtc").stdstringValue();
            isPreInitialized_ = true;
        }

        ASSERT(isPreInitialized_);
        WATCH(tle_.satellite_name);
        WATCH(tle_.tle_line1);
        WATCH(tle_.tle_line2);
        WATCH(wallClockSimStartTimeUtc_);

        initializePropagator();
        initializeTimeState();
    }
    else if (stage == inet::INITSTAGE_SINGLE_MOBILITY) {
        referenceSystem_ = GeographicReferenceSystemAccess().get();
        ASSERT(referenceSystem_ != nullptr);
        updateSatellitePosition();
        emitMobilityStateChangedSignal();
        refreshDisplay();
    }
}

void LeoSatMobility::setInitialPosition()
{
    // Initial position is computed after the geographic reference system becomes available.
}

void LeoSatMobility::initializePosition()
{
    MovingMobilityBase::initializePosition();
}

void LeoSatMobility::initializePropagator()
{
    double startmfe = -2880;
    double stopmfe = 2880;
    double deltamin = 1 / 600.0;
    SGP4Funcs::twoline2rv(
            (char *)tle_.get_tle_line1().c_str(),
            (char *)tle_.get_tle_line2().c_str(),
            'c',
            'd',
            'i',
            gravconsttype::wgs72,
            startmfe,
            stopmfe,
            deltamin,
            satrec_);
}

void LeoSatMobility::initializeTimeState()
{
    if (wallClockSimStartTimeUtc_.empty())
        throw cRuntimeError("LeoSatMobility::initializeTimeState - wallClockSimStartTimeUtc must be configured");

    tleEpoch_.year = (satrec_.epochyr >= 57) ? 1900 + satrec_.epochyr : 2000 + satrec_.epochyr;
    SGP4Funcs::days2mdhms_SGP4(
            satrec_.epochyr,
            satrec_.epochdays,
            tleEpoch_.mon,
            tleEpoch_.day,
            tleEpoch_.hour,
            tleEpoch_.min,
            tleEpoch_.sec);

    SGP4Funcs::jday_SGP4(
            tleEpoch_.year,
            tleEpoch_.mon,
            tleEpoch_.day,
            tleEpoch_.hour,
            tleEpoch_.min,
            tleEpoch_.sec,
            tleEpochJd_,
            tleEpochFrac_);

    std::istringstream iss(wallClockSimStartTimeUtc_);
    std::tm tmp = {};
    iss >> std::get_time(&tmp, "%Y-%m-%d-%H-%M-%S");
    if (iss.fail())
        throw cRuntimeError("LeoSatMobility::initializeTimeState - invalid wallClockSimStartTimeUtc '%s', expected YYYY-MM-DD-HH-MM-SS", wallClockSimStartTimeUtc_.c_str());

    wallClockStartTime_.year = tmp.tm_year + 1900;
    wallClockStartTime_.mon = tmp.tm_mon + 1;
    wallClockStartTime_.day = tmp.tm_mday;
    wallClockStartTime_.hour = tmp.tm_hour;
    wallClockStartTime_.min = tmp.tm_min;
    wallClockStartTime_.sec = tmp.tm_sec;

    SGP4Funcs::jday_SGP4(
            wallClockStartTime_.year,
            wallClockStartTime_.mon,
            wallClockStartTime_.day,
            wallClockStartTime_.hour,
            wallClockStartTime_.min,
            wallClockStartTime_.sec,
            wallClockSimStartTimeJd_,
            wallClockSimStartTimeFrac_);

    diffTleEpochWctMin_ = (wallClockSimStartTimeJd_ - tleEpochJd_) * 1440
                        + (wallClockSimStartTimeFrac_ - tleEpochFrac_) * 1440;
}

void LeoSatMobility::updateSatellitePosition()
{
    double t = diffTleEpochWctMin_ + simTime().dbl() / 60.0;

    double rArray[3];
    double vArray[3];
    bool ret = SGP4Funcs::sgp4(satrec_, t, rArray, vArray);
    if (!ret)
        throw cRuntimeError("LeoSatMobility::updateSatellitePosition - SGP4 propagation failed");

    std::vector<double> rTEME(rArray, rArray + 3);
    std::vector<double> vTEME(vArray, vArray + 3);

    double daysToAdd = std::trunc((wallClockSimStartTimeFrac_ + simTime().dbl()) / 86400.0);
    currentWallClockTimeFrac_ = wallClockSimStartTimeFrac_ + simTime().dbl() / 86400.0;
    currentWallClockTimeJd_ = wallClockSimStartTimeJd_ + daysToAdd;

    auto itrf = TEME_to_ITRF(currentWallClockTimeJd_, rTEME, vTEME, 0.0, 0.0, currentWallClockTimeFrac_);

    double latitude = 0;
    double longitude = 0;
    double altitude = 0;
    // TEME_to_ITRF returns Earth-fixed Cartesian coordinates in the ITRF family.
    // Here we treat ITRF2008 and WGS84 as equivalent for simulation purposes and
    // convert directly to geodetic coordinates on the WGS84 ellipsoid.
    WGS84_EARTH.Reverse(itrf.first[0] * 1000, itrf.first[1] * 1000, itrf.first[2] * 1000, latitude, longitude, altitude);
    inet::GeoCoord satelliteWgs84{inet::deg(latitude), inet::deg(longitude), inet::m(altitude)};

    EV_TRACE << "LeoSatMobility simTime(): " << simTime() << std::endl;
    EV_DEBUG << "LeoSatMobility sat_pos_wgs84: (lat(deg) " << satelliteWgs84.latitude
             << ", lon(deg) " << satelliteWgs84.longitude
             << ", alt(m) " << satelliteWgs84.altitude << ")" << std::endl;

    lastPosition = referenceSystem_->omnetFromWgs84(satelliteWgs84);

    EV_TRACE << "LeoSatMobility new lastPosition: " << lastPosition << std::endl;
}

void LeoSatMobility::handleSelfMessage(cMessage *message)
{
    MovingMobilityBase::handleSelfMessage(message);
}

void LeoSatMobility::move()
{
    updateSatellitePosition();
    lastVelocity = inet::Coord::ZERO;
    lastUpdate = simTime();
    nextChange = simTime() + updateInterval;

    EV_DEBUG << "LeoSatMobility moved SimTime: " << simTime() << std::endl;
    updateDisplayStringFromMobilityState();
}

void LeoSatMobility::setTle(const TLE& tle)
{
    tle_ = tle;
}

TLE LeoSatMobility::getTle() const
{
    return tle_;
}

void LeoSatMobility::finish()
{
    MovingMobilityBase::finish();
}

} // namespace simu5g
