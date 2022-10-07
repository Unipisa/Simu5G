//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package simu5g.nodes.mec.bgMecAppManager.bgModules;
import inet.mobility.contract.IMobility;
import inet.mobility.static.StationaryMobility;


module BgMecApp
{
}

module BgUe
{
    parameters:
        @networkNode();
        @display("i=device/pocketpc;bgb=160,100");
        string mobilityType = default("StationaryMobility");
    submodules:
        mobility: StationaryMobility {
            @display("p=28,30;is=s");
            initialZ = 0m;
//            initFromDisplayString = false;
//            updateFromDisplayString = false;
                  
        }
}
