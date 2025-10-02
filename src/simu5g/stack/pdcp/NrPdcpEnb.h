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

#ifndef _NRPDCPRRCENB_H_
#define _NRPDCPRRCENB_H_

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/LteDefs.h"
#include "simu5g/stack/pdcp/NrTxPdcpEntity.h"
#include "simu5g/stack/pdcp/LtePdcpEnbD2D.h"
#include "simu5g/stack/dualConnectivityManager/DualConnectivityManager.h"

namespace simu5g {

/**
 * @class NrPdcpEnb
 * @brief PDCP Layer
 *
 * This is the PDCP/RRC layer of NR Stack
 *
 */
class NrPdcpEnb : public LtePdcpEnbD2D
{

  protected:

    // Flag for enabling Dual Connectivity
    bool dualConnectivityEnabled_;

    // Reference to the Dual Connectivity Manager
    inet::ModuleRefByPar<DualConnectivityManager> dualConnectivityManager_;

    void initialize(int stage) override;

    /**
     * Analyze the packet and fill out its lteInfo.
     * @param pkt Incoming packet
     */
    void analyzePacket(inet::Packet *pkt) override;

    /**
     * Handler for um/am sap
     *
     * It performs the following steps:
     * - Decompresses the header, restoring the original packet
     * - Decapsulates the packet
     * - Sends the packet to the application layer
     *
     * @param pkt Incoming packet
     */
    void fromLowerLayer(cPacket *pkt) override;

    MacNodeId getDestId(inet::Ptr<FlowControlInfo> lteInfo) override;

    /*
     * Dual Connectivity support
     */
    bool isDualConnectivityEnabled() override { return dualConnectivityEnabled_; }

    // Send packet to the target node by invoking the Dual Connectivity manager
    void forwardDataToTargetNode(Packet *pkt, MacNodeId targetNode) override;

    // Receive packet from the source node. Called by the Dual Connectivity manager
    void receiveDataFromSourceNode(Packet *pkt, MacNodeId sourceNode) override;

  public:
    virtual void activeUeUL(std::set<MacNodeId> *ueSet);

};

} //namespace

#endif

