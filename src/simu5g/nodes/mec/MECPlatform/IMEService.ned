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


package simu5g.nodes.mec.MECPlatform;

//
// This module interface defines the structure that every
// MEC Service must implement (see, e.g., ~LocationService)
//
// TODO: obsolete, to be removed (see ~IMECService instead)
//
moduleinterface IMEService
{
    parameters:
        @display("i=block/app");
        string serviceName;

    gates:
        input meAppIn[];
        output meAppOut[];
}



