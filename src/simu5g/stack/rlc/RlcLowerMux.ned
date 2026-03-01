package simu5g.stack.rlc;

//
// Lower-layer RLC packet dispatcher.
//
// Dispatches MAC packets to the correct RX entity or TX entity,
// and collects TX entity output to route to the MAC layer.
//
simple RlcLowerMux
{
    parameters:
        @class("RlcLowerMux");
        @display("p=100,200;i=block/dispatch");

        @signal[receivedPacketFromLowerLayer];
        @statistic[receivedPacketFromLowerLayer](source="receivedPacketFromLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[sentPacketToLowerLayer];
        @statistic[sentPacketToLowerLayer](source="sentPacketToLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
    gates:
        input macIn;
        output macOut;
        output toUm;
        input fromUm;
        output toRxEntity[];          // dynamic: dispatches to RX entity in gates
        input fromTxEntity[];         // dynamic: receives from TX entity out gates
        output macToTxEntity[];       // dynamic: dispatches MAC SDU requests to TX entity macIn gates
        output toUpperMux;             // for forwarding D2D switch notifications to PDCP
}
