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

#ifndef DUALCONNECTIVITYMANAGER_H_
#define DUALCONNECTIVITYMANAGER_H_

#include <inet/common/ModuleRefByPar.h>

#include "common/LteCommon.h"
#include "x2/packet/X2ControlInfo_m.h"
#include "stack/dualConnectivityManager/X2DualConnectivityDataMsg.h"
#include "stack/pdcp_rrc/LtePdcpRrc.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

//
// DualConnectivityManager
//
class DualConnectivityManager : public cSimpleModule
{

  protected:

    // reference to PDCP layer
    inet::ModuleRefByPar<LtePdcpRrcBase> pdcp_;

    // X2 identifier
    X2NodeId nodeId_;

    // reference to the gates
    cGate *x2ManagerInGate_ = nullptr;
    cGate *x2ManagerOutGate_ = nullptr;

    void handleX2Message(cMessage *msg);

  public:

    void initialize() override;
    void handleMessage(cMessage *msg) override;

    // send a PDCP PDU to the X2 Manager
    void forwardDataToTargetNode(inet::Packet *pkt, MacNodeId targetNode);

    // receive PDCP PDU from X2 Manager and send it to the PDCP layer
    void receiveDataFromSourceNode(inet::Packet *pkt, MacNodeId sourceNode);
};

} //namespace

#endif /* DUALCONNECTIVITYMANAGER_H_ */

