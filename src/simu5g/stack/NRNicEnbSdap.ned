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

import simu5g.stack.sdap.NrRxSdapEntity;

//
// This module extends the Enb NIC to insert SDAP functionality (receiver-side only)
// between IP and PDCP layers, allowing QoS Flow management per 3GPP 5G specifications.
//
module NRNicEnbSdap extends NrNicEnb
{
    parameters:
        nrSdapDisabled = false;
        int numDrbs = 1;
        @display("bgb=694,443");

    submodules:

        nrRxSdapEntity: NrRxSdapEntity {
            numDrbs = parent.numDrbs;
            @display("p=493,67");
        }

    connections:
        ip2nic.stackNic <--> nrRxSdapEntity.DataPort;

        //# SDAP <-> PDCP
        nrRxSdapEntity.stackSdap <--> pdcp.DataPort;

}

