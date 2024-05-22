
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

package simu5g.nodes.mec.VirtualisationInfrastructureManager;

//
// This module represents the background application running on the 
// Virtualisation Infrastructure Manager (VIM). It is intended for
// simulating background tasks that utilize CPU, RAM, and disk resources.
// The parameters allow configuring the allocation of these resources.
//
module BackgroundApp
{
    double ram @unit("B") = default(10MB);
    double cpu = default(500);
    double disk @unit("B") = default(10MB);
}
