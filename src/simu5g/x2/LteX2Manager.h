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

#ifndef LTE_LTEX2MANAGER_H_
#define LTE_LTEX2MANAGER_H_

#include <inet/common/ModuleRefByPar.h>
#include <inet/networklayer/common/L3AddressResolver.h>

#include "common/LteCommon.h"
#include "x2/packet/LteX2Message.h"
#include "x2/packet/X2ControlInfo_m.h"
#include "x2/X2AppClient.h"
#include "common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

class LteX2Manager : public cSimpleModule
{

    // X2 identifier
    X2NodeId nodeId_;

    // reference to the LTE Binder module
    inet::ModuleRefByPar<Binder> binder_;

    // "interface table" for data gates
    // for each X2 message type, this map stores the index of the gate vector data
    // where the destination of that message is connected to
    std::map<LteX2MessageType, int> dataInterfaceTable_;

    // "interface table" for x2 gates
    // for each destination ID, this map stores the index of the gate vector x2
    // where the X2AP for that destination is connected to
    std::map<X2NodeId, int> x2InterfaceTable_;

  protected:

    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;

    virtual void fromStack(inet::Packet *pkt);
    virtual void fromX2(inet::Packet *pkt);

};

} //namespace

#endif /* LTE_LTEX2MANAGER_H_ */

