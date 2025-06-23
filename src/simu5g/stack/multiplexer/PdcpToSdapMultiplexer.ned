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

package simu5g.stack.multiplexer;

simple PdcpToSdapMultiplexer
{
    parameters:
        int numDrbs = default(4);  // configurable number of DRBs (matches PDCP instances)
        @display("i=block/join");

    gates:
        // Mux
        output sdapOut;
        input sdapIn;
        input pdcpIn[numDrbs];
        output pdcpOut[numDrbs];
}
