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

#ifndef __LTEHANDOVERMANAGER_H_
#define __LTEHANDOVERMANAGER_H_

#include "stack/ip2nic/IP2Nic.h"
#include "common/LteCommon.h"
#include "x2/packet/X2ControlInfo_m.h"
#include "stack/handoverManager/X2HandoverControlMsg.h"
#include "stack/handoverManager/X2HandoverDataMsg.h"

class IP2Nic;

//
// LteHandoverManager
//
class LteHandoverManager : public omnetpp::cSimpleModule
{

  protected:

    // X2 identifier
    X2NodeId nodeId_;

    // reference to the gates
    omnetpp::cGate* x2Manager_[2];

    // reference to the PDCP layer
    IP2Nic* ip2nic_;

    // flag for seamless/lossless handover
    bool losslessHandover_;

    void handleX2Message(omnetpp::cPacket* pkt);

  public:
    LteHandoverManager() {}
    virtual ~LteHandoverManager() {}

    virtual void initialize() override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;

    // send handover command on X2 to the eNB
    void sendHandoverCommand(MacNodeId ueId, MacNodeId enb, bool startHo);

    // receive handover command on X2 from the source eNB
    void receiveHandoverCommand(MacNodeId ueId, MacNodeId eEnb, bool startHo);

    // send an IP datagram to the X2 Manager
    void forwardDataToTargetEnb(inet::Packet* datagram, MacNodeId targetEnb);

    // receive data from X2 message and send it to the X2 Manager
    void receiveDataFromSourceEnb(inet::Packet* datagram, MacNodeId sourceEnb);
};

#endif /* __LTEHANDOVERMANAGER_H_ */
