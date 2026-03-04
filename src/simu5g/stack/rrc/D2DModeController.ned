package simu5g.stack.rrc;

//
// Controls D2D mode switching flow control.
//
// When a UE switches between infrastructure and direct D2D communication
// modes, this module coordinates the draining of the old-mode RLC TX
// entity's queue before allowing the new-mode entity to start transmitting.
//
simple D2DModeController
{
    parameters:
        @class("D2DModeController");
        @display("i=block/control");
}
