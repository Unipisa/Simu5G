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


package simu5g.stack;

import simu5g.stack.sdap.NrTxSdapEntity;

// This module extends the UE NIC to insert SDAP functionality (transmit-side only)
// between IP and PDCP layers, allowing QoS Flow management per 3GPP 5G specifications.
//
module NRNicUeSdap extends NrNicUe
{
    parameters:
		nrSdapDisabled = false;
		int numDrbs = 1;
        @display("bgb=694,443");

    submodules:

        nrTxSdapEntity: NrTxSdapEntity {
            numDrbs = parent.numDrbs;
            @display("p=466,141");
        }


    connections:
        ip2nic.stackNic <--> nrTxSdapEntity.DataPort;

        //# SDAP <-> PDCP
        nrTxSdapEntity.stackSdap <--> pdcp.DataPort;
}
