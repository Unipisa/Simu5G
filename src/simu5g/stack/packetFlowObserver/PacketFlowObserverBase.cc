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

#include <sstream>
#include <inet/common/ModuleAccess.h>
#include <inet/common/packet/Packet.h>
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/stack/pdcp/LtePdcp.h"
#include "simu5g/stack/rlc/LteRlcDefs.h"
#include "simu5g/stack/rlc/packet/LteRlcPdu_m.h"
#include "simu5g/common/LteControlInfo.h"
#include "PacketFlowObserverBase.h"

namespace simu5g {

void PacketFlowObserverBase::initialize(int stage)
{
    if (stage == INITSTAGE_SIMU5G_POSTLOCAL) {
        LteMacBase *mac = getModuleFromPar<LteMacBase>(par("macModule"), this);
        nodeType_ = mac->getNodeType();
        harqProcesses_ = mac->harqProcesses();
        pfmType = par("pfmType").stringValue();

        // Subscribe to PDCP signals
        cModule *pdcpModule = getModuleFromPar<cModule>(par("pdcpModule"), this);
        bool isNrObserver = par("isNrObserver").boolValue();
        simsignal_t pdcpSduSentSignal = registerSignal(isNrObserver ? "pdcpSduSentNr" : "pdcpSduSent");
        simsignal_t pdcpSduReceivedSignal = registerSignal("pdcpSduReceived");
        pdcpModule->subscribe(pdcpSduSentSignal, this);
        pdcpModule->subscribe(pdcpSduReceivedSignal, this);
    }
}

void PacketFlowObserverBase::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details)
{
    static simsignal_t pdcpSduSentSignal = registerSignal("pdcpSduSent");
    static simsignal_t pdcpSduSentNrSignal = registerSignal("pdcpSduSentNr");
    static simsignal_t pdcpSduReceivedSignal = registerSignal("pdcpSduReceived");

    if (signalID == pdcpSduSentSignal || signalID == pdcpSduSentNrSignal) {
        auto pkt = check_and_cast<inet::Packet *>(obj);
        insertPdcpSdu(pkt);
    }
    else if (signalID == pdcpSduReceivedSignal) {
        auto pkt = check_and_cast<inet::Packet *>(obj);
        receivedPdcpSdu(pkt);
    }
}


void PacketFlowObserverBase::resetDiscardCounter()
{
    pktDiscardCounterTotal_ = { 0, 0 };
}

} //namespace
