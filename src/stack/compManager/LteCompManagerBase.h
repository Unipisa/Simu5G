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

#ifndef LTE_LTECOMPMANAGERBASE_H_
#define LTE_LTECOMPMANAGERBASE_H_

#include "common/LteCommon.h"
#include "stack/mac/LteMacEnb.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "x2/packet/X2ControlInfo_m.h"
#include "stack/compManager/X2CompMsg.h"
#include "stack/compManager/X2CompRequestIE.h"
#include "stack/compManager/X2CompReplyIE.h"

namespace simu5g {

using namespace omnetpp;

enum CompNodeType {
    COMP_CLIENT,
    COMP_CLIENT_COORDINATOR,
    COMP_COORDINATOR
};

//
// LteCompManagerBase
// Base class for CoMP manager modules.
// To add a new CoMP algorithm, extend this class and redefine pure virtual methods
//
class LteCompManagerBase : public cSimpleModule
{
  protected:
    // X2 identifier
    X2NodeId nodeId_;

    // reference to the gates
    cGate *x2ManagerInGate_ = nullptr;
    cGate *x2ManagerOutGate_ = nullptr;

    // reference to the MAC layer
    opp_component_ptr<LteMacEnb> mac_;

    // number of available bands
    int numBands_;

    // period between two coordination instances
    double coordinationPeriod_;

    /// Self messages
    cMessage *compClientTick_ = nullptr;
    cMessage *compCoordinatorTick_ = nullptr;

    // Comp Node Type specification (client, client and coordinator, coordinator only)
    CompNodeType nodeType_;

    // Last received usable bands
    UsableBands usableBands_;

    // ID of the coordinator
    X2NodeId coordinatorId_;

    // IDs of the eNB that are slaves of this master node
    std::vector<X2NodeId> clientList_;

    // statistics
    static simsignal_t compReservedBlocksSignal_;

    void runClientOperations();
    void runCoordinatorOperations();
    void handleX2Message(inet::Packet *pkt);
    void sendClientRequest(X2CompRequestIE *requestIe);
    void sendCoordinatorReply(X2NodeId clientId, X2CompReplyIE *replyIe);

    virtual void provisionalSchedule() = 0;  // run the provisional scheduling algorithm (client side)
    virtual void doCoordination() = 0;       // run the coordination algorithm (coordinator side)

    virtual X2CompRequestIE *buildClientRequest() = 0;
    virtual void handleClientRequest(inet::IntrusivePtr<X2CompMsg> compMsg) = 0;

    virtual X2CompReplyIE *buildCoordinatorReply(X2NodeId clientId) = 0;
    virtual void handleCoordinatorReply(inet::IntrusivePtr<X2CompMsg> compMsg) = 0;

    void setUsableBands(UsableBands& usableBands);

  public:
    void initialize() override;
    void handleMessage(cMessage *msg) override;
};

} //namespace

#endif /* LTE_LTECOMPMANAGERBASE_H_ */

