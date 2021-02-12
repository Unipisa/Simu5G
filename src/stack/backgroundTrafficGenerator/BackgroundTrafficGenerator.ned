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

package simu5g.stack.backgroundTrafficGenerator;

module BackgroundTrafficGenerator
{
    parameters:
        @display("i=block/source");
        int numBgUes = default(0);

    submodules:
        bgUE[numBgUes]: BackgroundUe {
            @display("p=77,118");
        }
        manager: BackgroundTrafficManager {
            @display("p=77,41");
        }
}

simple BackgroundTrafficManager 
{
    parameters:
        @display("i=block/control");
        @class("BackgroundTrafficManager");
}    
    