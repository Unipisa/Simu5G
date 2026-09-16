//
//                  Simu5G
//
// Authors: Mohamed Seliem (University College Cork)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.sdap;

//
// Interface for SDAP protocol implementations.
//
moduleinterface INrSdap
{
    parameters:
        string nodeRole @enum("UE", "gNB");  // Node role: "UE" or "gNB" - determines reflective QoS behavior
        bool useReflectiveQos; // UE only
        string reflectiveQosTableModule;  // UE only: path to ReflectiveQosTable module (empty = disabled)

    gates:
        input upperLayerIn;     // From IP layer
        output upperLayerOut;   // To IP layer
        input pdcpIn;           // From PDCP layer
        output pdcpOut;         // To PDCP layer
}
