
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

package simu5g.stack.pdcp_rrc;

//
// The LtePdcpRrcEnbD2D module represents the PDCP/RRC layer for an LTE eNodeB with Device-to-Device (D2D) communication capabilities.
// This module extends the basic LtePdcpRrcEnb by adding support for managing D2D-capable user equipments, including handling 
// D2D mode switching and enhanced packet flow management. It plays a key role in handling packet flows between upper and lower layers 
// and supporting the specific requirements of D2D-enabled LTE networks.
//
/**
 * @brief D2D-capable eNodeB PDCP/RRC layer of the LTE stack.
 *
 * This module represents the PDCP/RRC layer in an eNodeB within the LTE Stack with support for Device-to-Device communication.
 */
simple LtePdcpRrcEnbD2D extends LtePdcpRrcEnb
{
    @class("LtePdcpRrcEnbD2D");
}

