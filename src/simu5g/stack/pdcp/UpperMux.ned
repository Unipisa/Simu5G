package simu5g.stack.pdcp;

//
// Upper-layer PDCP packet dispatcher.
//
// Receives IP datagrams from the upper layer, looks up (or triggers creation of)
// the appropriate TX PDCP entity by DrbKey, and dispatches the packet to it.
// Collects processed SDUs from RX entities and sends them to the upper layer.
//
simple UpperMux
{
    parameters:
        @class("UpperMux");
        @display("p=80,100;i=block/dispatch");
    gates:
        input upperLayerIn;
        output upperLayerOut;
        output toTxEntity[];       // dynamic: dispatches to TX entity in gates
        input fromRxEntity[];      // dynamic: receives from RX entity out gates
}
