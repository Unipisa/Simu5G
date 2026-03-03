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

#include <sstream>
#include <inet/common/ModuleAccess.h>
#include <inet/common/packet/Packet.h>
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/stack/rlc/LteRlcDefs.h"
#include "simu5g/stack/rlc/packet/LteRlcPdu_m.h"
#include "simu5g/stack/packetFlowObserver/PacketFlowSignals.h"
#include "simu5g/stack/mac/packet/LteMacPdu.h"
#include "simu5g/common/LteControlInfo.h"
#include "PacketFlowObserverBase.h"

namespace simu5g {

void PacketFlowObserverBase::initialize(int stage)
{
    if (stage == INITSTAGE_SIMU5G_POSTLOCAL) {
        LteMacBase *mac = getModuleFromPar<LteMacBase>(par("macModule"), this);
        nodeType_ = mac->getNodeType();
        harqProcesses_ = (nodeType_ == UE) ? UE_TX_HARQ_PROCESSES : ENB_TX_HARQ_PROCESSES;
        pfmType = par("pfmType").stringValue();

        // Subscribe to PDCP signals
        cModule *pdcpModule = getModuleFromPar<cModule>(par("pdcpModule"), this);
        bool isNrObserver = par("isNrObserver").boolValue();
        simsignal_t pdcpSduSentSignal = registerSignal(isNrObserver ? "pdcpSduSentNr" : "pdcpSduSent");
        simsignal_t pdcpSduReceivedSignal = registerSignal("pdcpSduReceived");
        pdcpModule->subscribe(pdcpSduSentSignal, this);
        pdcpModule->subscribe(pdcpSduReceivedSignal, this);

        // Subscribe to RLC signal (on compound RLC module, catches dynamic UmTxEntity children)
        cModule *rlcModule = getModuleFromPar<cModule>(par("rlcModule"), this);
        simsignal_t rlcPduCreatedSignal = registerSignal("rlcPduCreated");
        rlcModule->subscribe(rlcPduCreatedSignal, this);

        // Subscribe to MAC signals
        cModule *macModule = getModuleFromPar<cModule>(par("macModule"), this);
        macModule->subscribe(registerSignal("macPduAcked"), this);
        macModule->subscribe(registerSignal("macPduDiscarded"), this);
        macModule->subscribe(registerSignal("rlcPduDiscarded"), this);
        macModule->subscribe(registerSignal("grantSent"), this);
        macModule->subscribe(registerSignal("ulMacPduArrived"), this);
    }
}

void PacketFlowObserverBase::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    static simsignal_t pdcpSduSentSignal = registerSignal("pdcpSduSent");
    static simsignal_t pdcpSduSentNrSignal = registerSignal("pdcpSduSentNr");
    static simsignal_t pdcpSduReceivedSignal = registerSignal("pdcpSduReceived");
    static simsignal_t rlcPduCreatedSignal = registerSignal("rlcPduCreated");
    static simsignal_t macPduAckedSignal = registerSignal("macPduAcked");
    static simsignal_t macPduDiscardedSignal = registerSignal("macPduDiscarded");
    static simsignal_t rlcPduDiscardedSignal = registerSignal("rlcPduDiscarded");
    static simsignal_t grantSentSignal = registerSignal("grantSent");
    static simsignal_t ulMacPduArrivedSignal = registerSignal("ulMacPduArrived");

    if (signalID == pdcpSduSentSignal || signalID == pdcpSduSentNrSignal) {
        auto pkt = check_and_cast<inet::Packet *>(obj);
        insertPdcpSdu(pkt);
    }
    else if (signalID == pdcpSduReceivedSignal) {
        auto pkt = check_and_cast<inet::Packet *>(obj);
        receivedPdcpSdu(pkt);
    }
    else if (signalID == rlcPduCreatedSignal) {
        auto info = check_and_cast<RlcPduSignalInfo *>(obj);
        insertRlcPdu(info->drbId, info->rlcPdu, info->burstStatus);
    }
    else if (signalID == macPduAckedSignal) {
        auto info = check_and_cast<MacPduSignalInfo *>(obj);
        macPduArrived(info->macPdu);
    }
    else if (signalID == macPduDiscardedSignal) {
        auto info = check_and_cast<MacPduSignalInfo *>(obj);
        discardMacPdu(info->macPdu);
    }
    else if (signalID == rlcPduDiscardedSignal) {
        auto info = check_and_cast<RlcDiscardSignalInfo *>(obj);
        discardRlcPdu(info->drbId, info->rlcSno);
    }
    else if (signalID == grantSentSignal) {
        auto info = check_and_cast<GrantSignalInfo *>(obj);
        grantSent(info->nodeId, info->grantId);
    }
    else if (signalID == ulMacPduArrivedSignal) {
        auto info = check_and_cast<GrantSignalInfo *>(obj);
        ulMacPduArrived(info->nodeId, info->grantId);
    }
}


void PacketFlowObserverBase::resetDiscardCounter()
{
    pktDiscardCounterTotal_ = { 0, 0 };
}

} //namespace
