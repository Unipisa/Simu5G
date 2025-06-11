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
// SDAP Transmit Entity.
//
// This module implements the transmit-side SDAP functionality for the UE.
// It receives IP packets from upper layers and maps them to appropriate QoS Flows,
// attaches SDAP headers if necessary, and forwards them to the PDCP layer.
//
// Parameters:
//  - qfiToDrbMapping: optional static mapping from QFI to DRB for simulation purposes.
//
// Gates:
//  - DataPort: connects to the IP layer.
//  - stackSdap[]: connects to the PDCP layer Instances.
//
simple NrTxSdapEntity
{
	parameters:
	    string qfiContextFile = default("");  // external config file
	    int numDrbs = default(1); 										// this to allow multiple DRBs
        @display("i=block/tx");
    gates:
        inout DataPort;				// To/from IP layer
        inout stackSdap[numDrbs];   // To/from PDCP layer
}
