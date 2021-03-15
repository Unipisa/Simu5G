// 
//                           Simu5G
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.nodes.backgroundCell;

import inet.mobility.static.StationaryMobility;
import lte.stack.backgroundTrafficGenerator.BackgroundTrafficGenerator;

// 
// BackgroundCell Module
//
// This background cell implements only one carrier.
// If you want to simulate multiple carriers, deploy multiple background cells in the same position
// and change carrierFrequency parameter  
module BackgroundCell
{
    parameters:
        @display("i=device/antennatower;bgb=430,238");
        
        string bgTrafficManagerType = default("BackgroundCellTrafficManager");
        
    submodules:
        // mobility module for the background base station
        mobility: StationaryMobility {
            @display("p=100,38;is=s");
        }
        
        // perform the operations of the BackgroundCell
        bgBaseStation: BackgroundBaseStation {
            @display("p=100,113,row");
        }

        // traffic generator for the background UEs within the BackgroundCell
        bgTrafficGenerator: BackgroundTrafficGenerator {
            @display("p=261,113,row");
            bgTrafficManagerType = bgTrafficManagerType; 
        }
}


simple BackgroundBaseStation 
{
    parameters:
        @display("i=block/control");
        @class("BackgroundBaseStation");
        
        // transmission power
        double txPower = default(46);
        		
        // transmission direction
        string txDirection = default("OMNI");
        
        // transmission angle
        double txAngle = default(0);
        
        // flag to indicate whether this is a NR base station 
        bool isNr = default(false); 
        
        // operating carrier
        int numerologyIndex = default(0);
        
        // operating carrier
        double carrierFrequency @unit(GHz) = default(2GHz);
        
        // carrier bandwidth
        int numBands = default(6);
}    
    