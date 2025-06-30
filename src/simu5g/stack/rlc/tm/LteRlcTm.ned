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


package simu5g.stack.rlc.tm;

//
// This module implements the functionalities of the Transparent Mode (TM) of the
// Radio Link Control (RLC) layer.
//
simple LteRlcTm
{
    parameters:
        @display("i=block/wheelbarrow");
        int queueSize @unit(B) = default(2MiB);              // RLC SDU queue size (0: unlimited)

        //# SDU-level statistics
        @signal[rlcPacketLossUl];
        @statistic[rlcPacketLossUl](title="rlc Packet Loss"; unit=""; source="rlcPacketLossUl"; record=mean,sum,vector);
        @signal[rlcPacketLossDl];
        @statistic[rlcPacketLossDl](title="rlc Packet Loss"; unit=""; source="rlcPacketLossDl"; record=mean,sum,vector);

        //# PDU-level statistics
        @signal[receivedPacketFromUpperLayer];
        @statistic[receivedPacketFromUpperLayer](source="receivedPacketFromUpperLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[receivedPacketFromLowerLayer];
        @statistic[receivedPacketFromLowerLayer](source="receivedPacketFromLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[sentPacketToUpperLayer];
        @statistic[sentPacketToUpperLayer](source="sentPacketToUpperLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[sentPacketToLowerLayer];
        @statistic[sentPacketToLowerLayer](source="sentPacketToLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
    gates:
        //#
        //# Gates connecting RLC and TM
        //#
        inout TM_Sap_up;    // Gate to PDCP
        inout TM_Sap_down;    // Gate to RLC Mux
}

