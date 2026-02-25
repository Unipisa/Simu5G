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

#ifndef _LTE_PACKETFLOWSIGNALS_H_
#define _LTE_PACKETFLOWSIGNALS_H_

#include <omnetpp.h>
#include "simu5g/common/LteCommon.h"
#include "simu5g/mec/utils/MecCommon.h"

namespace simu5g {

class LteRlcUmDataPdu;

/**
 * Signal value struct for RLC PDU creation events (rlcPduCreated signal).
 * Carries the DRB ID, a pointer to the RLC PDU, and the burst status.
 */
struct RlcPduSignalInfo : public omnetpp::cObject
{
    DrbId drbId;
    const LteRlcUmDataPdu *rlcPdu;
    RlcBurstStatus burstStatus;

    RlcPduSignalInfo(DrbId drbId, const LteRlcUmDataPdu *rlcPdu, RlcBurstStatus burstStatus)
        : drbId(drbId), rlcPdu(rlcPdu), burstStatus(burstStatus) {}
};

/**
 * Signal value struct for RLC PDU discard events (rlcPduDiscarded signal).
 * Carries the DRB ID and the RLC sequence number.
 */
struct RlcDiscardSignalInfo : public omnetpp::cObject
{
    DrbId drbId;
    unsigned int rlcSno;

    RlcDiscardSignalInfo(DrbId drbId, unsigned int rlcSno)
        : drbId(drbId), rlcSno(rlcSno) {}
};

/**
 * Signal value struct for grant and UL MAC PDU arrival events
 * (grantSent and ulMacPduArrived signals).
 * Carries the MAC node ID and the grant ID.
 */
struct GrantSignalInfo : public omnetpp::cObject
{
    MacNodeId nodeId;
    unsigned int grantId;

    GrantSignalInfo(MacNodeId nodeId, unsigned int grantId)
        : nodeId(nodeId), grantId(grantId) {}
};

} //namespace

#endif
