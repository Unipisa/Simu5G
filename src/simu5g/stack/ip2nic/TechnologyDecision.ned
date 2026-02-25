package simu5g.stack.ip2nic;

//
// Selects between LTE and NR technology for outgoing packets based on
// Type of Service field and dual connectivity configuration.
// Sits above ~Ip2Nic in the outgoing packet path.
//
simple TechnologyDecision like ITechnologyDecision
{
    parameters:
        @display("i=block/fork");
    gates:
        input upperLayerIn;
        output lowerLayerOut;
}
