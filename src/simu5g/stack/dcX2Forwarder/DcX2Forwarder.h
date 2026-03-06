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

#ifndef DCX2FORWARDER_H_
#define DCX2FORWARDER_H_

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/x2/packet/X2ControlInfo_m.h"
#include "simu5g/stack/dcX2Forwarder/X2DualConnectivityDataMsg.h"

namespace simu5g {

using namespace omnetpp;
using namespace inet;

//
// DcX2Forwarder
//
class DcX2Forwarder : public cSimpleModule
{
  protected:

    // X2 identifier
    X2NodeId nodeId_;

    // reference to the gates
    cGate *x2ManagerInGate_ = nullptr;
    cGate *x2ManagerOutGate_ = nullptr;

    void handleX2Message(cMessage *msg);

  protected:

    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;

    // send a PDCP PDU to the X2 Manager
    void forwardDataToTargetNode(inet::Packet *pkt, MacNodeId targetNode);

    // receive PDCP PDU from X2 Manager and send it to the PDCP layer
    void receiveDataFromSourceNode(inet::Packet *pkt, MacNodeId sourceNode);
};

} //namespace

#endif /* DCX2FORWARDER_H_ */

