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
// Interface for the connection control of the ~IRrc layer: the node's control-plane
// entry point, the one module of the node that the control plane of other nodes
// talks to. At a base station (~ConnectionControlEnb) it owns the node's GTP-U
// tunnel endpoint ids and programs the node's tunnel endpoints.
//
moduleinterface IConnectionControl
{
    parameters:
        @display("i=block/control");
}
