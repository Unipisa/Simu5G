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

#ifndef _LTE_MULTIHOPD2D_H_
#define _LTE_MULTIHOPD2D_H_

#include <string.h>
#include <omnetpp.h>
#include <inet/common/ModuleRefByPar.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include "apps/d2dMultihop/MultihopD2DPacket_m.h"
#include "apps/d2dMultihop/statistics/MultihopD2DStatistics.h"
#include "apps/d2dMultihop/eventGenerator/EventGenerator.h"
#include "stack/mac/LteMacBase.h"
#include "stack/phy/LtePhyBase.h"

namespace simu5g {

using namespace omnetpp;

class EventGenerator;

class MultihopD2D : public cSimpleModule
{
    static uint16_t numMultihopD2DApps;  // counter of apps (used for assigning the ids)

  protected:
    enum {
        KIND_SELF_SENDER = 1000,
        KIND_RELAY,
        KIND_TRICKLE_TIMER
    };
    uint16_t senderAppId_;             // unique identifier of the application within the network
    uint16_t localMsgId_ = 0;          // least-significant bits for the identifier of the next message
    inet::B msgSize_;

    double selfishProbability_;         // if = 0, the node is always collaborative
    int ttl_;                           // if < 0, do not use hops to limit the flooding
    double maxBroadcastRadius_;         // if < 0, do not use radius to limit the flooding
    simtime_t maxTransmissionDelay_;    // if > 0, when a new message has to be transmitted, choose an offset between 0 and maxTransmissionDelay_

    /*
     * Trickle parameters
     */
    bool trickleEnabled_;
    unsigned int k_;
    simtime_t I_;
    std::map<unsigned int, inet::Packet *> last_;
    std::map<unsigned int, unsigned int> counter_;
    /***************************************************/

    std::map<uint32_t, bool> relayedMsgMap_;  // indicates if a received message has been relayed before

    int localPort_;
    int destPort_;
    inet::L3Address destAddress_;
    inet::UdpSocket socket;

    cMessage *selfSender_ = nullptr;

    ModuleRefByPar<EventGenerator> eventGen_;          // reference to the eventGenerator
    ModuleRefByPar<LtePhyBase> ltePhy_;                // reference to the LtePhy
    MacNodeId lteNodeId_;               // LTE Node Id
    MacNodeId lteCellId_;               // LTE Cell Id

    // local statistics
    static simsignal_t d2dMultihopGeneratedMsgSignal_;
    static simsignal_t d2dMultihopSentMsgSignal_;
    static simsignal_t d2dMultihopRcvdMsgSignal_;
    static simsignal_t d2dMultihopRcvdDupMsgSignal_;
    static simsignal_t d2dMultihopTrickleSuppressedMsgSignal_;

    // reference to the statistics manager
    ModuleRefByPar<MultihopD2DStatistics> stat_;

    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;
    void finish() override;

    void markAsReceived(uint32_t msgId);      // store the msg id in the set of received messages
    bool isAlreadyReceived(uint32_t msgId);   // returns true if the given msg has already been received
    void markAsRelayed(uint32_t msgId);       // set the corresponding entry in the set as relayed
    bool isAlreadyRelayed(uint32_t msgId);   // returns true if the given msg has already been relayed
    bool isWithinBroadcastArea(inet::Coord srcCoord, double maxRadius);

    virtual void sendPacket();
    virtual void handleRcvdPacket(cMessage *msg);
    virtual void handleTrickleTimer(cMessage *msg);
    virtual void relayPacket(cMessage *msg);

  public:
    MultihopD2D();
    ~MultihopD2D() override;

    virtual void handleEvent(unsigned int eventId);
};

} //namespace

#endif

