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

#ifndef __SIMU5G_LOCALGEOMAPCANVASVISUALIZER_H_
#define __SIMU5G_LOCALGEOMAPCANVASVISUALIZER_H_

#include "inet/common/geometry/common/GeographicCoordinateSystem.h"
#include "inet/visualizer/canvas/scene/GeoMapCanvasVisualizer.h"

namespace simu5g {

// A geographic map visualizer for simulations whose scene coordinates use a
// local geographic frame instead of the ECEF frame expected by INET's map
// projection.
class LocalGeoMapCanvasVisualizer : public inet::visualizer::GeoMapCanvasVisualizer
{
  protected:
    const inet::IGeographicCoordinateSystem *coordinateSystem_ = nullptr;

    void initialize(int stage) override;
    void configureCanvasProjection() override;
};

} // namespace simu5g

#endif
