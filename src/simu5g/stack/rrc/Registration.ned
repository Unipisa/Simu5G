//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.rrc;

//
// RRC Registration — registers the node with the Binder, sets up
// the network interface, and joins multicast groups.
//
simple Registration
{
    parameters:
        @display("i=block/control");
        @class(Registration);

        string interfaceTableModule;
        string interfaceName = default("cellular");
        string routingTableModule;
        string binderModule = default("binder");
}
