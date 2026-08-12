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

package simu5g.visualizer;

import inet.mobility.static.StationaryMobility;
import inet.visualizer.canvas.mobility.MobilityCanvasVisualizer;
import inet.visualizer.canvas.scene.GeoHorizonCanvasVisualizer;
import inet.visualizer.canvas.scene.NetworkNodeCanvasVisualizer;


// A geographic marker associated with a local simulation frame.
module LocalScene
{
    parameters:
        @display("i=misc/town;is=vs");

    submodules:
        mobility: StationaryMobility {
            @display("p=60,60;is=s");
        }
}

// A nested world-map canvas showing selected root-network nodes and the local
// scene marker in their geographic positions.
module WorldMapVisualizer
{
    parameters:
        @display("i=misc/globe;is=vs;bgb=197,227");
    submodules:
        map: LocalGeoMapCanvasVisualizer {
            @display("p=59,40;is=vs");
        }
        networkNodeVisualizer: NetworkNodeCanvasVisualizer {
            @display("p=59,85;is=vs");
        }
        satelliteVisualizer: MobilityCanvasVisualizer {
            @display("p=59,130;is=vs");
        }
        geoHorizon: GeoHorizonCanvasVisualizer {
            @display("p=59,175;is=vs");
        }
        scene: LocalScene {
            @display("p=135,40");
        }
}
