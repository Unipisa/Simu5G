package simu5g.stack.ip2nic;

//
// Selects between LTE and NR technology for outgoing packets.
// Sits above ~Ip2Nic in the outgoing packet path.
//
// When both LTE and NR stacks are available, the decision is made by
// evaluating a configurable expression. Two expressions are provided:
// - dcUseNrCondition: used when dual connectivity is enabled and both stacks are available
// - useNrCondition: used when DC is not enabled but both stacks are available
//
// Available variables: typeOfService (int), packetOrdinal (int per-flow counter).
// Return: true = use NR, false = use LTE.
//
simple TechnologyDecision like ITechnologyDecision
{
    parameters:
        @display("i=block/fork");
        string nodeType;
        string binderModule = default("binder");

        // DC case: both LTE and NR stacks available, dual connectivity enabled.
        object dcUseNrCondition = default(expr(
            typeOfService >= 20 ? (packetOrdinal % 2 != 0)
                                : typeOfService >= 10
        ));

        // Non-DC case: both LTE and NR stacks available, DC not enabled.
        object useNrCondition = default(expr(typeOfService >= 10));

    gates:
        input upperLayerIn;
        output lowerLayerOut;
}
