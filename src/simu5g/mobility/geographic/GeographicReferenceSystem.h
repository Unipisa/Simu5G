#ifndef __SIMU5G_GEOGRAPHICREFERENCESYSTEM_H_
#define __SIMU5G_GEOGRAPHICREFERENCESYSTEM_H_

#include <proj.h>

#include <omnetpp.h>

#include "inet/common/INETDefs.h"
#include "inet/common/InitStages.h"
#include "inet/common/geometry/common/Coord.h"
#include "space_veins/modules/utility/WGS84Coord.h"
#include "veins/base/utils/FindModule.h"

namespace simu5g {

class GeographicReferenceSystem : public omnetpp::cSimpleModule
{
  protected:
    inet::Coord referenceOmnetCoord_;
    space_veins::WGS84Coord referenceWgs84_;
    PJ_COORD referenceWgs84ProjCart_;

    PJ_CONTEXT *pjCtx_ = nullptr;
    PJ *wgs84ToWgs84CartesianProjection_ = nullptr;
    PJ *wgs84CartesianToTopocentricProjection_ = nullptr;

    void initialize(int stage) override;

  public:
    ~GeographicReferenceSystem() override;

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    const inet::Coord& getReferenceOmnetCoord() const { return referenceOmnetCoord_; }
    const space_veins::WGS84Coord& getReferenceWgs84() const { return referenceWgs84_; }
    PJ_COORD getReferenceWgs84ProjCart() const { return referenceWgs84ProjCart_; }

    inet::Coord omnetFromWgs84(const space_veins::WGS84Coord& wgs84Coord) const;
};

class GeographicReferenceSystemAccess {
  public:
    GeographicReferenceSystem *get()
    {
        return veins::FindModule<GeographicReferenceSystem *>::findGlobalModule();
    }
};

} // namespace simu5g

#endif
