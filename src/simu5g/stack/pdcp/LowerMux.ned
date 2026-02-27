package simu5g.stack.pdcp;

//
// Lower-layer PDCP packet dispatcher.
//
// Receives PDCP PDUs from RLC, looks up the appropriate RX PDCP entity
// by DrbKey, and dispatches the packet to it. Collects processed PDUs
// from TX entities and routes them to the correct RLC output gate
// (LTE RLC, NR RLC, or DC manager) based on PdcpOutputRoutingTag.
//
// Also handles Dual Connectivity manager input (bypass entity dispatch)
// and D2D mode switch notifications.
//
simple LowerMux
{
    parameters:
        @class("LowerMux");
        @display("p=80,200;i=block/dispatch");
        string binderModule = default("binder");
        bool isNR = default(false);
        bool hasD2DSupport = default(false);
    gates:
        input rlcIn;
        output rlcOut;
        input nrRlcIn @loose;
        output nrRlcOut @loose;
        input dcManagerIn @loose;
        output dcManagerOut @loose;

        output toRxEntity[];           // dynamic: dispatches to RX entity in gates
        input fromTxEntity[];          // dynamic: receives from TX entity out gates
        output toBypassTxEntity[];     // dynamic: dispatches to bypass TX entity in gates
        input fromBypassRxEntity[];    // dynamic: receives from bypass RX entity out gates
}
