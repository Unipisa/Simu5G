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

package simu5g.stack.rlc.am;

//
// TODO auto-generated module
//
simple NrAmRxQueue extends AmRxQueue
{
    parameters:
        @class(NrAmRxQueue);
        //These values should be configured by RRC
        double t_Reassembly @unit(s)=default(80ms);
        double t_StatusProhibit @unit(s)=default(100ms);
        int AM_Window_Size =default(2048); //Only 2048 or 131072
        double throughputInterval @unit(s) = default(1s);
}
