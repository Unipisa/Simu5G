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

package lte.nodes.mec.VirtualisationInfrastructureManager;


//
//# VirtualisationInfrastructureManager
//	
//	Simple Module for handling the availability of virtualised resources on the ME Host.
//	From  ETSI GS MEC 003, it is responsable for allocating, managing and releasing virtualised 
//  resources of the virtualisation infrastructure;
// 

simple VirtualisationInfrastructureManager
{
    parameters:
        @display("i=block/process");
        
        
    gates:
        input virtualisationManagerIn;
        output virtualisationManagerOut;
}



