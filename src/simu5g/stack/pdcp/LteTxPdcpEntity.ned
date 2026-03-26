//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.pdcp;

//
// Handles operations related to the LTE PDCP (Packet Data Convergence Protocol)
// layer on the transmitting side of a specific connection. This module manages the creation,
// sequencing, header compression, and metadata association for PDCP PDUs before they are passed
// down to the RLC layer.
//
simple LteTxPdcpEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/segm");
}

