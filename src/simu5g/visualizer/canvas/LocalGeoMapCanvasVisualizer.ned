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

package simu5g.visualizer.canvas;

import inet.visualizer.canvas.scene.GeoMapCanvasVisualizer;

// Draws an equirectangular geographic map for a simulation that uses a local
// IGeographicCoordinateSystem scene frame.
simple LocalGeoMapCanvasVisualizer extends GeoMapCanvasVisualizer
{
    parameters:
        string coordinateSystemModule; // local geographic coordinate system used by mobility modules
        @class(LocalGeoMapCanvasVisualizer);
}
