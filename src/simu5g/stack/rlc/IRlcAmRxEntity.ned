//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.rlc;

//
// Receiving side of an RLC AM entity: extends the generic RX interface with
// the gates over which it hands control PDUs to the co-located transmitting
// side (see RlcAmEntityBase).
//
moduleinterface IRlcAmRxEntity extends IRlcRxEntity
{
    gates:
        output feedbackOut;    // Received STATUS PDUs -> TX-side ARQ
        output statusOut;  // Own STATUS reports -> TX for transmission
}
