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

import simu5g.stack.multiplexer.RlcToMacMultiplexer;
import simu5g.stack.pdcp.NrPdcpEnb;
import simu5g.stack.rlc.LteRlc;
import simu5g.stack.sdap.NrRxSdapEntity;

//
// This module extends the Enb NIC to insert DRB functionality
// allowing QoS Flow management per 3GPP 5G specifications.
//
module NRNicEnbDrb extends LteNicEnbD2D
{
    parameters:
        nrSdapDisabled = false;
        drbDisabled = false;

        int numDrbs; // this to allow multiple DRBs
        @display("bgb=694,443");
        lteChannelModelType = default("NrChannelModel_3GPP38_901");

        packetFlowManager.typename = default("NrPacketFlowManagerGnb");


    submodules:

        nrRxSdapEntity: NrRxSdapEntity {
            numDrbs = parent.numDrbs;
            @display("p=245,67");
        }

        // CASE SDAP active: → multiple DRBs activated
        nrPdcp[numDrbs]: NrPdcpEnb if !nrSdapDisabled {
            @display("p=245,149,r,10");
            drbIndex = index;  // assign DRB ID
        }

		// RLC Layer
        nrRlc[numDrbs]: LteRlc {
            @display("p=245,231,r,10");
            drbIndex = index;
            d2dCapable = parent.d2dCapable;
            *.macModule = "^.^.mac";
            *.packetFlowManagerModule = parent.hasRniSupport ? "^.^.packetFlowManager" : "";
            um.nodeType = parent.nodeType;

        }

        rlcToMacMultiplexer: RlcToMacMultiplexer {
            numDrbs = parent.numDrbs;
            @display("p=245,314");
        }
    connections:
        ip2nic.stackNic <--> nrRxSdapEntity.DataPort;


		// New multi-DRB connections when SDAP is enabled
        for i=0..numDrbs-1 {
            nrRxSdapEntity.stackSdap[i] <--> nrPdcp[i].DataPort;

            //# PDCP <-> RLC
            nrPdcp[i].AM_Sap++ <--> nrRlc[i].AM_Sap;
            nrPdcp[i].UM_Sap++ <--> nrRlc[i].UM_Sap;
            nrPdcp[i].TM_Sap++ <--> nrRlc[i].TM_Sap;

            //# RLC <-> MUX
            nrRlc[i].RLC_to_MAC --> rlcToMacMultiplexer.rlcIn[i];
            nrRlc[i].MAC_to_RLC <-- rlcToMacMultiplexer.rlcOut[i];
        }

        rlcToMacMultiplexer.macOut --> mac.RLC_to_MAC;
        rlcToMacMultiplexer.macIn <-- mac.MAC_to_RLC;

}

