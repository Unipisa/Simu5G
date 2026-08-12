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

#include "simu5g/visualizer/LocalGeoMapCanvasVisualizer.h"

#include "inet/common/ModuleAccess.h"
#include "inet/common/geometry/common/CanvasProjection.h"
#include "inet/common/geometry/common/IMapProjection.h"
#include "inet/common/geometry/common/Wgs84.h"

namespace simu5g {

using namespace omnetpp;

namespace {

// Adapts INET's ECEF-based equirectangular projection to an arbitrary
// IGeographicCoordinateSystem scene frame.
class LocalEquirectangularProjection : public inet::IMapProjection
{
  protected:
    const inet::IGeographicCoordinateSystem *coordinateSystem_;
    inet::EquirectangularProjection projection_;

    inet::Coord sceneToEcef(const inet::Coord& point) const
    {
        return inet::wgs84::geodeticToEcef(coordinateSystem_->computeGeographicCoordinate(point));
    }

  public:
    LocalEquirectangularProjection(const inet::IGeographicCoordinateSystem *coordinateSystem,
            const inet::EquirectangularProjection& projection) :
        coordinateSystem_(coordinateSystem), projection_(projection)
    {
    }

    inet::Coord applyForward(const inet::Coord& scenePoint) const override
    {
        return projection_.applyForward(sceneToEcef(scenePoint));
    }

    inet::Coord applyInverse(const inet::Coord& projectedPoint) const override
    {
        auto geographicCoordinate = inet::wgs84::ecefToGeodetic(projection_.applyInverse(projectedPoint));
        return coordinateSystem_->computeSceneCoordinate(geographicCoordinate);
    }

    cFigure::Point computeCanvasDirection(const inet::Coord& point, const inet::Coord& direction,
            double& depth, DirectionProjection directionProjection, const inet::RotationMatrix& rotation,
            const cFigure::Point& scale) const override
    {
        auto ecefPoint = sceneToEcef(point);
        auto ecefDirection = sceneToEcef(point + direction) - ecefPoint;
        return projection_.computeCanvasDirection(ecefPoint, ecefDirection, depth,
                directionProjection, rotation, scale);
    }
};

} // namespace

Define_Module(LocalGeoMapCanvasVisualizer);

void LocalGeoMapCanvasVisualizer::initialize(int stage)
{
    inet::visualizer::GeoMapCanvasVisualizer::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
        coordinateSystem_ = inet::findModuleFromPar<inet::IGeographicCoordinateSystem>(par("coordinateSystemModule"), this);
}

void LocalGeoMapCanvasVisualizer::configureCanvasProjection()
{
    if (!displayMap)
        return;
    if (coordinateSystem_ == nullptr)
        throw cRuntimeError("Geographic coordinate system module not found on path '%s' defined by par 'coordinateSystemModule'",
                par("coordinateSystemModule").stringValue());

    auto canvasProjection = inet::CanvasProjection::getCanvasProjection(visualizationTargetModule->getCanvas());
    canvasProjection->setMapProjection(new LocalEquirectangularProjection(coordinateSystem_, projection));
    canvasProjection->setRotation(inet::RotationMatrix());

    double longitudeSpan = projection.getLongitudeSpan();
    double latitudeSpan = projection.getMaxLatitude() - projection.getMinLatitude();
    canvasProjection->setScale(cFigure::Point(projection.getMapWidth() / longitudeSpan,
            -projection.getMapHeight() / latitudeSpan));
    canvasProjection->setTranslation(cFigure::Point(
            -projection.getMinLongitude() * projection.getMapWidth() / longitudeSpan,
            projection.getMaxLatitude() * projection.getMapHeight() / latitudeSpan));
    if (clipFigures)
        canvasProjection->setClipRect(cFigure::Rectangle(0, 0, projection.getMapWidth(), projection.getMapHeight()));
    else
        canvasProjection->clearClipRect();
}

} // namespace simu5g
