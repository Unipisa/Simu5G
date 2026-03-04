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
        object drbConfig;  // JSON array of DRB configurations: [{drb, ue, qfiList}, ...]
        bool useReflectiveQos; // UE only
        string reflectiveQosTableModule;  // UE only: path to ReflectiveQosTable module (empty = disabled)

    gates:
        input upperLayerIn;     // from IP layer
        output upperLayerOut;   // to IP layer
        input pdcpIn;           // from PDCP layer
        output pdcpOut;         // to PDCP layer
}
