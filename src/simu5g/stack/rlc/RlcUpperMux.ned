package simu5g.stack.rlc;

//
// Upper-layer RLC packet dispatcher.
//
// Dispatches upper-layer packets to the correct TX entity by DrbKey,
// and collects RX entity output to forward to the upper layer.
//
simple RlcUpperMux
{
    parameters:
        @class("RlcUpperMux");
        @display("p=100,50;i=block/dispatch");
        string binderModule = default("binder");

        @signal[receivedPacketFromUpperLayer];
        @statistic[receivedPacketFromUpperLayer](source="receivedPacketFromUpperLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[sentPacketToUpperLayer];
        @statistic[sentPacketToUpperLayer](source="sentPacketToUpperLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
    gates:
        input upperLayerIn;
        output upperLayerOut;
        input fromLowerMux;            // for D2D switch notifications forwarded to PDCP
        output toTxEntity[];           // dynamic: dispatches to TX entity in gates
        input fromRxEntity[];          // dynamic: receives from RX entity out gates
}
