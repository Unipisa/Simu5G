
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
// This module represents a D2D-capable User Equipment (UE) PDCP/RRC layer in the LTE stack.
// It extends the basic LTE PDCP/RRC functionality with D2D communication capabilities,
// allowing direct communication between UEs without routing traffic through the base station.
// The module integrates with other LTE layers including MAC and PHY to support D2D operations.
//
simple LtePdcpRrcUeD2D extends LtePdcpRrcUe
{
    parameters:
        @class("LtePdcpRrcUeD2D");
        /**
         * @brief D2D-capable PDCP/RRC layer module for UE in LTE networks.
         *
         * This module extends the basic PDCP/RRC layer to enable Device-to-Device (D2D) communication in LTE networks.
         * It handles both normal uplink communication to the eNB and direct D2D communication between UEs.
         * The module includes functionality for handling mode switches between D2D and uplink modes.
         */
}

