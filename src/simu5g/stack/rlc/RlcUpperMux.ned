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
    gates:
        input upperLayerIn;
        output upperLayerOut;
        output toUm;
        input fromUm;
}
