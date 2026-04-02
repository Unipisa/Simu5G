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


package simu5g.stack.mac;

//
// Extends the ~LteMacBase module by providing specific MAC functions
// at the User Equipment (UE) side.
// For uplink connections, this module manages the initiation of the Random Access procedure (RAC)
// and the sending of Buffer Status Reports (BSRs) to the eNodeBs. Once it receives an uplink scheduling
// grant from the eNodeB, it is responsible for managing the local connections' scheduler and
// constructing MAC Protocol Data Units (PDUs) to be transmitted to the physical layer.
//
simple LteMacUe extends LteMacBase
{
    parameters:
        @class("LteMacUe");
        string collectorModule = default("");

        int numPreambles = default(64);        // number of RACH preambles available for contention-based random access
        int maxRacAttempts = default(10);      // preambleTransMax; max RAC attempts (0 = give up after first failure)
        int racBackoffMin = default(0);        // min random backoff (TTIs) after failed RAC
        int racBackoffMax = default(20);       // max random backoff (TTIs) after failed RAC (0 = no backoff, instant retry)
        int raResponseWindow = default(3);     // ra-ResponseWindow: TTIs to wait before retrying RAC
        int retxBsrTimer = default(40);        // retxBSR-Timer: BSR retransmission timer (TTIs; 0 = disabled)
}

