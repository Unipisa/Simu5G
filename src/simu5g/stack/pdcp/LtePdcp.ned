//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//


package simu5g.stack.pdcp;

//
// The LtePdcp module implements the functionalities of the Packet Data
// Convergence Protocol (PDCP). It enables header compression and maps incoming IP packets
// to different Radio Link Control (RLC) modes. This module dynamically creates
// ~LteTxPdcpEntity and ~LteRxPdcpEntity submodules to handle individual transmission/reception connections
//
//
// Thin facade module that provides the entity management API (create, delete,
// lookup) for external callers (RRC, HandoverController, MAC). Delegates to
// ~UpperMux and ~LowerMux for actual packet handling and entity storage.
//
simple LtePdcp
{
    parameters:
        @class("PdcpMux");
        @display("p=180,150;i=block/cogwheel");
        bool isNR = default(false);
        bool hasD2DSupport = default(false);
}

