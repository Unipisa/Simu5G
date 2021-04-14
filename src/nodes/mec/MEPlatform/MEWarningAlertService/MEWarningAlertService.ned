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

package simu5g.nodes.mec.MEPlatform.MEWarningAlertService;

import simu5g.nodes.mec.MEPlatform.IMEService;

//
// MEWarningAlertService connected to the MEWarningAlertApp
//
// When receives the INFO_UEAPP from the UEWarningAlertApp checks if the UE is within the danger area 
// defined by the quadrilateral dangerEdgeA - dangerEdgeB - dangerEdgeC - dangerEdgeD
// if UE IS INSIDE then REPLAY with INFO_MEAPP WarningAlertPacket with danger == TRUE else danger == FALSE

simple MEWarningAlertService like IMEService
{
    parameters:
        
        double dangerEdgeAx @unit("m");
        double dangerEdgeAy @unit("m");
        double dangerEdgeAz @unit("m") = default(0m);
        double dangerEdgeBx @unit("m");
        double dangerEdgeBy @unit("m");
        double dangerEdgeBz @unit("m") = default(0m);
        double dangerEdgeCx @unit("m");
        double dangerEdgeCy @unit("m");
        double dangerEdgeCz @unit("m") = default(0m);
        double dangerEdgeDx @unit("m");
        double dangerEdgeDy @unit("m");
        double dangerEdgeDz @unit("m") = default(0m);
        
    gates:
       
        input meAppIn[];
        output meAppOut[];     
}
