// 
//                           Simu5G
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.stack.backgroundTrafficGenerator;

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
    