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
// Implements the SDAP Protocol.
//
// For PDUs received from the PDCP layer, extracts SDAP headers (if present),
// restores the associated QoS Flow based on QFI, and forwards the resulting SDUs
// to the upper IP layer.
//
// For SDUs received from the IP layer, performs QFI-to-DRB mapping,
// encapsulates the data with SDAP headers if needed, and forwards the packets
// to the PDCP layer.
//
// Parameters:
//  - qfiToDrbMapping: optional static mapping from QFI to DRB for simulation purposes.
//
// Gates:
//  - DataPort: connects to the IP layer.
//  - stackSdap: connects to the PDCP layer.
//
moduleinterface INrSdap
{
    parameters:
        int numDrbs; // number of DRBs
        string nodeRole @enum("UE", "gNB");  // Node role: "UE" or "gNB" - determines reflective QoS behavior
    gates:
        inout DataPort;     // To/from IP
        inout stackSdap;     // To/from PDCP
}
