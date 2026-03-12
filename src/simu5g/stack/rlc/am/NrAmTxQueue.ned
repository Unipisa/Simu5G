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
simple NrAmTxQueue extends AmTxQueue
{
    parameters:
        @class(NrAmTxQueue);
        int AM_Window_Size =default(2048); //For 12 bit SN; use 131072 for 18 bit SN. The SN should be configured by the RRC
        int pollByte @unit(B)=default(125kB);
        int pollPDU =default(64);
        int maxRtxThreshold = default(4);
        double t_PollRetransmit @unit(s) = default(80ms);
        string amModule = default("^.am");
}
