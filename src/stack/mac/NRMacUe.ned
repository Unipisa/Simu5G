//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.mac;

//
// This module provides functionalities for the Medium Access Control (MAC) of the
// New Radio (NR) protocol stack at the User Equipment (UE) side. In particular, it extends
// the ~LteMacUeD2D module by redefining functions to support different numerology
// parameterizations.
//
simple NRMacUe extends LteMacUeD2D
{
    parameters:
        @class("NRMacUe");
}
