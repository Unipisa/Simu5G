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

#ifndef __MECREQUESTAPP_H_
#define __MECREQUESTAPP_H_

#include <inet/common/ModuleRefByPar.h>
#include <inet/networklayer/common/L3Address.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>

#include "apps/mecRequestResponseApp/packets/MecRequestResponsePacket_m.h"
#include "apps/mecRequestResponseApp/packets/MigrationTimer_m.h"
#include "stack/phy/NRPhyUe.h"

namespace simu5g {

using namespace omnetpp;

class MecRequestApp : public cSimpleModule
{
    inet::UdpSocket socket;
    simtime_t period_;
    int localPort_;
    int destPort_;
    std::string sourceSymbolicAddress_;
    inet::L3Address destAddress_;

    inet::ModuleRefByPar<NRPhyUe> nrPhy_;

    unsigned int sno_;
    MacNodeId bsId_;
    unsigned int appId_;

    bool enableMigration_;

    // scheduling
    cMessage *selfSender_ = nullptr;

    static simsignal_t requestSizeSignal_;
    static simsignal_t requestRTTSignal_;
    static simsignal_t recvResponseSnoSignal_;

  public:
    ~MecRequestApp() override;

  protected:

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;
    void finish() override;

    void sendRequest();
    void recvResponse(cMessage *msg);
};

} //namespace

#endif

