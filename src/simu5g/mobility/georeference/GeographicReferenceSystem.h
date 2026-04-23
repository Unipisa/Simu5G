#ifndef __SIMU5G_GEOGRAPHICREFERENCESYSTEM_H_
#define __SIMU5G_GEOGRAPHICREFERENCESYSTEM_H_

#include <proj.h>

#include <omnetpp.h>

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
    PJ_COORD referenceWgs84ProjCart_;

    PJ_CONTEXT *pjCtx_ = nullptr;
    PJ *wgs84ToWgs84CartesianProjection_ = nullptr;
    PJ *wgs84CartesianToTopocentricProjection_ = nullptr;

    void initialize(int stage) override;

  public:
    ~GeographicReferenceSystem() override;

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    const inet::Coord& getReferenceOmnetCoord() const { return referenceOmnetCoord_; }
    const inet::GeoCoord& getReferenceWgs84() const { return referenceWgs84_; }
    PJ_COORD getReferenceWgs84ProjCart() const { return referenceWgs84ProjCart_; }

    inet::Coord omnetFromWgs84(const inet::GeoCoord& wgs84Coord) const;
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
