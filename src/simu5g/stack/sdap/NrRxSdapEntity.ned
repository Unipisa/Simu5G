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
// SDAP Receive Entity.
//
// This module implements the receive-side SDAP functionality for the UE.
// It receives PDUs from the PDCP layer, extracts the SDAP header if present,
// restores the corresponding QoS Flow based on the QFI, and forwards the
// packet to the upper IP layer.
//
// Parameters:
//  - qfiToDrbMapping: optional static mapping from QFI to DRB for simulation purposes.
//
// Gates:
//  - DataPort: connects to the IP layer.
//  - stackSdap: connects to the PDCP layer.
//
simple NrRxSdapEntity
{
    parameters:
	string qfiContextFile = default("");  // external config file
	int numDrbs = default(1); // this to allow multiple DRBs
	@display("i=block/rx");
    gates:
        inout DataPort;     // To/from IP
        inout stackSdap[numDrbs];     // To/from PDCP
}
