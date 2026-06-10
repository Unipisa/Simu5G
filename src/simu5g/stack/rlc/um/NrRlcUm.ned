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

package simu5g.stack.rlc.um;

//
// TODO auto-generated module
//
simple NrRlcUm extends LteRlcUm
{
    parameters:
        @class(NrRlcUm);
        @signal[wastedGrantedBytes];
        @signal[requestedPDUSize];
        @signal[sentPDUSize];
        @statistic[wastedGrantedBytes](title="Bytes granted by MAC that are not used because the buffer is empty"; unit="B"; source="wastedGrantedBytes"; record=sum);
        @statistic[requestedPDUSize](title="Bytes requested by MAC"; unit="B"; source="requestedPDUSize"; record=vector);
        @statistic[sentPDUSize](title="Bytes sent to MAC"; unit="B"; source="sentPDUSize"; record=vector);
        
     
}
