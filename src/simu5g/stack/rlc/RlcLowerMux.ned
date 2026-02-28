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
    gates:
        input macIn;
        output macOut;
        output toUm;
        input fromUm;
}
