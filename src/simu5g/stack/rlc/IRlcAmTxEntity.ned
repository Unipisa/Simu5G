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
// Transmitting side of an RLC AM entity: extends the generic TX interface
// with the gates over which the co-located receiving side hands it control
// PDUs (see RlcAmEntityBase).
//
moduleinterface IRlcAmTxEntity extends IRlcTxEntity
{
    gates:
        input feedbackIn;    // STATUS PDUs received from the peer (to ARQ)
        input statusIn;  // Locally generated STATUS reports (to transmit)
}
