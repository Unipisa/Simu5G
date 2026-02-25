package simu5g.stack.ip2nic;

//
// Module interface for technology decision modules that select between
// LTE and NR technology for outgoing packets.
//
moduleinterface ITechnologyDecision
{
    parameters:
        @display("i=block/fork");
        string nodeType;
    gates:
        input upperLayerIn;
        output lowerLayerOut;
}
