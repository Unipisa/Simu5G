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

#include "simu5g/stack/pdcp/NrTxPdcpEntity.h"
#include "simu5g/common/LteControlInfoTags_m.h"

namespace simu5g {

Define_Module(NrTxPdcpEntity);


void NrTxPdcpEntity::initialize(int stage)
{
    LteTxPdcpEntity::initialize(stage);
}

void NrTxPdcpEntity::deliverPdcpPdu(Packet *pkt)
{
    auto lteInfo = pkt->getTag<FlowControlInfo>();
    if (getNodeTypeById(pdcp_->nodeId_) == UE) {
        EV << NOW << " NrTxPdcpEntity::deliverPdcpPdu - LCID[" << lteInfo->getLcid() << "] - sending packet to lower layer" << endl;
        LteTxPdcpEntity::deliverPdcpPdu(pkt);
    }
    else { // ENODEB
        MacNodeId destId = lteInfo->getDestId();
        if (getNodeTypeById(destId) != UE)
            throw cRuntimeError("NrTxPdcpEntity::deliverPdcpPdu - destination must be a UE");

        if (!pdcp_->isDualConnectivityEnabled()) {
            EV << NOW << " NrTxPdcpEntity::deliverPdcpPdu - LCID[" << lteInfo->getLcid() << "] - the destination is a UE. Sending packet to lower layer" << endl;
            LteTxPdcpEntity::deliverPdcpPdu(pkt);
        }
        else {
            bool useNR = pkt->getTag<TechnologyReq>()->getUseNR();

            if (!useNR) {
                EV << NOW << " NrTxPdcpEntity::deliverPdcpPdu - LCID[" << lteInfo->getLcid() << "] useNR[" << useNR << "] - the destination is a UE. Sending packet to lower layer." << endl;
                LteTxPdcpEntity::deliverPdcpPdu(pkt);
            }
            else { // useNR
                EV << NOW << " NrTxPdcpEntity::deliverPdcpPdu - LCID[" << lteInfo->getLcid() << "] - the destination is under the control of a secondary node" << endl;
                MacNodeId secondaryNodeId = pdcp_->binder_->getSecondaryNode(pdcp_->nodeId_);
                ASSERT(secondaryNodeId != NODEID_NONE);
                ASSERT(secondaryNodeId != pdcp_->nodeId_);
                pdcp_->forwardDataToTargetNode(pkt, secondaryNodeId);
            }
        }
    }
}

} //namespace

