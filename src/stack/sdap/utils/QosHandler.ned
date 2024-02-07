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

package simu5g.stack.sdap.utils;

//
// TODO auto-generated module
//
moduleinterface IQosHandler
{
}

simple QosHandler like IQosHandler
{
    parameters:
        int v2xQfi = default(1);
        //int v2x5Qi = default(84);//GBR 84
        int v2x5Qi = default(86);//GBR 86

        int voipQfi = default(2);
        int voip5Qi = default(2);//GBR

//        int videoQfi = default(3);
//        int video5Qi = default(7);//NGBR

		int videoQfi = default(3);
        int video5Qi = default(74);//GBR

        int dataQfi = default(4);
        int data5Qi = default(9);//NGBR
   
        
        int networkControlQfi = default(7);
        int networkControl5Qi = default(82);

        int gamingQfiToRadioBearer = default(1);
        int videoQfiToRadioBearer = default(1);
        int voipQfiToRadioBearer = default(1);
        int dataQfiToRadioBearer = default(1);
        int networkControlQfiToRadioBearer = default(1);
        @display("i=block/classifier");
}

simple QosHandlerUE extends QosHandler
{
    parameters:
        @class("QosHandlerUE");
        @display("i=block/classifier");

}

simple QosHandlerGNB extends QosHandler
{
    parameters:
        @class("QosHandlerGNB");
        @display("i=block/classifier");

}

simple QosHandlerUPF extends QosHandler
{
    parameters:
        @class("QosHandlerUPF");
        @display("i=block/classifier");

}
