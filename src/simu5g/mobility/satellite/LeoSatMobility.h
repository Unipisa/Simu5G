#ifndef __SIMU5G_LEOSATMOBILITY_H_
#define __SIMU5G_LEOSATMOBILITY_H_

#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

#include "inet/common/INETDefs.h"
#include "inet/common/InitStages.h"
#include "inet/common/geometry/common/GeographicCoordinateSystem.h"
#include "inet/mobility/base/MovingMobilityBase.h"

#include "simu5g/common/GeoUtils.h"
#include "simu5g/mobility/georeference/GeographicReferenceSystem.h"
#include "simu5g/mobility/satellite/SGP4.h"
#include "simu5g/mobility/satellite/TLE.h"

namespace simu5g {

class LeoSatMobility : public inet::MovingMobilityBase
{
  protected:
    struct DateTime {
        int year;
        int mon;
        int day;
        int hour;
        int min;
        double sec;
    };

    bool isPreInitialized_ = false;

    elsetrec satrec_;
    TLE tle_;
    GeographicReferenceSystem *referenceSystem_ = nullptr;

    std::string wallClockSimStartTimeUtc_;
    DateTime wallClockStartTime_;
    double wallClockSimStartTimeJd_ = std::numeric_limits<double>::quiet_NaN();
    double wallClockSimStartTimeFrac_ = std::numeric_limits<double>::quiet_NaN();
    DateTime tleEpoch_;
    double tleEpochJd_ = std::numeric_limits<double>::quiet_NaN();
    double tleEpochFrac_ = std::numeric_limits<double>::quiet_NaN();
    double currentWallClockTimeJd_ = std::numeric_limits<double>::quiet_NaN();
    double currentWallClockTimeFrac_ = std::numeric_limits<double>::quiet_NaN();
    double diffTleEpochWctMin_ = std::numeric_limits<double>::quiet_NaN();

  protected:
    void initialize(int stage) override;
    void setInitialPosition() override;
    void initializePosition() override;
    void handleSelfMessage(omnetpp::cMessage *message) override;
    void move() override;
    void finish() override;

    void updateSatellitePosition();
    void initializePropagator();
    void initializeTimeState();

  public:
    LeoSatMobility() = default;

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    void preInitialize(TLE tle, std::string wallClockSimStartTimeUtc);
    void setTle(const TLE& tle);
    TLE getTle() const;

    const inet::Coord& getCurrentPosition() override { return lastPosition; }
    const inet::Coord& getCurrentVelocity() override { return lastVelocity; }
    const inet::Quaternion& getCurrentAngularPosition() override { return inet::Quaternion::IDENTITY; }
    const inet::Coord& getCurrentAcceleration() override { return inet::Coord::ZERO; }
};

class LeoSatMobilityAccess {
  protected:
    static LeoSatMobility *findRecursively(omnetpp::cModule *module)
    {
        if (module == nullptr)
            return nullptr;
        if (auto *leoSatMobility = dynamic_cast<LeoSatMobility *>(module))
            return leoSatMobility;
        for (omnetpp::cModule::SubmoduleIterator it(module); !it.end(); ++it) {
            if (auto *found = findRecursively(*it))
                return found;
        }
        return nullptr;
    }

  public:
    LeoSatMobility *get(omnetpp::cModule *host)
    {
        auto *leoSatMobility = findRecursively(host);
        ASSERT(leoSatMobility != nullptr);
        return leoSatMobility;
    }
};

} // namespace simu5g

#endif
