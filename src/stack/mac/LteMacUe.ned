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


package simu5g.stack.mac;

//
// This module extends the ~LteMacBase module by providing specific MAC functions
// at the User Equipment (UE) side.
// For uplink connections, this module manages the initiation of the Random Access procedure (RAC)
// and the sending of Buffer Status Reports (BSRs) to the eNodeBs. Once it receives an uplink scheduling
// grant from the eNodeB, it is responsible for managing the local connections' scheduler and
// constructing MAC Protocol Data Units (PDUs) to be transmitted to the physical layer.
//
simple LteMacUe extends LteMacBase
{
    parameters:
        @class("LteMacUe");
        string collectorModule = default("");
}

