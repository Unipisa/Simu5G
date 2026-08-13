#ifndef __SIMU5G_GEOGRAPHICREFERENCESYSTEM_H_
#define __SIMU5G_GEOGRAPHICREFERENCESYSTEM_H_

#include <omnetpp.h>

// Temporarily hide INET's NaN macro while parsing GeographicLib headers
#ifdef NaN
  #pragma push_macro("NaN")
  #undef NaN
  #define GEOREF_RESTORE_NAN
#endif

#include "GeographicLib/Geocentric.hpp"
#include "GeographicLib/LocalCartesian.hpp"

// Restore NaN macro if it existed
#ifdef GEOREF_RESTORE_NAN
  #pragma pop_macro("NaN")
  #undef GEOREF_RESTORE_NAN
#endif

#include "inet/common/INETDefs.h"
#include "inet/common/InitStages.h"
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/geometry/common/GeographicCoordinateSystem.h"

namespace simu5g {

class GeographicReferenceSystem : public omnetpp::cSimpleModule
{
  protected:
    inet::Coord referenceOmnetCoord_;
    inet::GeoCoord referenceWgs84_ = inet::GeoCoord::NIL;
    inet::Coord referenceEcefCoord_ = inet::Coord::NIL;
    GeographicLib::Geocentric earth_;
    GeographicLib::LocalCartesian localFrame_;

    void initialize(int stage) override;

  public:
    GeographicReferenceSystem();

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    const inet::Coord& getReferenceOmnetCoord() const { return referenceOmnetCoord_; }
    const inet::GeoCoord& getReferenceWgs84() const { return referenceWgs84_; }
    const inet::Coord& getReferenceEcefCoord() const { return referenceEcefCoord_; }

    inet::Coord omnetFromWgs84(const inet::GeoCoord& wgs84Coord) const;
    inet::GeoCoord wgs84FromOmnet(const inet::Coord& omnetCoord) const;
};

class GeographicReferenceSystemAccess {
  protected:
    static GeographicReferenceSystem *findRecursively(omnetpp::cModule *module)
    {
        if (module == nullptr)
            return nullptr;
        if (auto *referenceSystem = dynamic_cast<GeographicReferenceSystem *>(module))
            return referenceSystem;
        for (omnetpp::cModule::SubmoduleIterator it(module); !it.end(); ++it) {
            if (auto *found = findRecursively(*it))
                return found;
        }
        return nullptr;
    }

  public:
    GeographicReferenceSystem *get()
    {
        return findRecursively(omnetpp::cSimulation::getActiveSimulation()->getSystemModule());
    }
};

} // namespace simu5g

#endif
