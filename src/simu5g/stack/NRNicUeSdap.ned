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

import simu5g.stack.sdap.INrSdap;
import simu5g.stack.sdap.common.IReflectiveQosTable;


// This module extends the UE NIC to insert SDAP functionality
// between IP and PDCP layers, allowing QoS Flow management per 3GPP 5G specifications.
//
module NRNicUeSdap extends NrNicUe
{
    parameters:
        hasNrSdap = true;
        bool hasReflectiveQosTable = default(false);  // Enable/disable reflective QoS table
        int numDrbs = 1;
        @display("bgb=694,443");

    submodules:

        sdap: <default("NrSdap")> like INrSdap {
            numDrbs = parent.numDrbs;
            nodeRole = "UE";
            reflectiveQosTableModule = parent.hasReflectiveQosTable ? "^.reflectiveQosTable" : "";
            @display("p=245,67");
        }

        // Conditional Reflective QoS Table for UE
        reflectiveQosTable: <default("ReflectiveQosTable")> like IReflectiveQosTable if hasReflectiveQosTable {
            @display("p=185,200;is=s");
        }


    connections:
        ip2nic.stackNic <--> sdap.DataPort;

        //# SDAP <-> PDCP
        sdap.stackSdap <--> pdcp.DataPort;
}
