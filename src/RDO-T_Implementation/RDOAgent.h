//
//#ifndef _UEAPP_H_
//#define _UEAPP_H_
//
//#include <fstream>
//#include <sstream>
//#include <string>
//#include <map>
//#include <vector>
//#include <cmath>
//#include <omnetpp.h>
//#include "inet/common/packet/Packet.h"
//#include "inet/common/TimeTag_m.h"
//#include "inet/networklayer/common/L3AddressTag_m.h"
//#include "RDOAgent.h"
//#include "RDOMessage_m.h"
//
//using namespace omnetpp;
//using namespace inet;
//
//// Constants for attack configuration
//const int ATTACK_TYPE_NORMAL = 0;
//const int ATTACK_TYPE_DOS = 5;
//const int ATTACK_TYPE_SYBIL = 4;
//const int ATTACK_TYPE_FALSIFIED = 3;
//
//class UeApp : public cSimpleModule
//{
//  private:
//    // Basic module parameters
//    int mySenderId;
//    int myAttackType;  // 0 = Normal, 3 = Falsified, 4 = Sybil, 5 = DoS
//    bool isAttacker = false;
//    bool isTrusted = false;  // Flag indicating if this node is trusted
//    double myAttackSeverity = 0.0;
//    simtime_t myAttackStartTime;
//    simtime_t myAttackEndTime;
//
//    // Operational parameters
//    cMessage *periodicMsgEvent;
//    cMessage *attackMsgEvent;
//    cMessage *rdoUpdateEvent;  // Added for RDO
//    int seqNumber = 0;
//
//    // Statistics
//    cOutVector packetSentVector;
//    cOutVector attackRateVector;
//    cOutVector rdoStateVector;  // Records state evolution
//    cOutVector rdoGradientVector;  // Records gradient values
//
//    // List of fake IDs for Sybil attack
//    std::vector<int> fakeIds;
//
//    // RDO-related parameters
//    bool rdoEnabled = false;
//    RDOAgent *rdoAgent = nullptr;  // RDO agent for this node
//    std::map<int, double> neighborStates;  // States of neighbors
//    std::vector<int> trustedNeighbors;  // List of trusted neighbors
//
//  protected:
//    virtual void initialize() override;
//    virtual void handleMessage(cMessage *msg) override;
//    virtual void finish() override;
//
//    // Helper methods
//    void loadAttackNodesFromCSV(const std::string& filename);
//    void loadTrustedNodesFromCSV(const std::string& filename);
//    void startAttack();
//    void stopAttack();
//
//    // Message sending methods
//    void sendNormalMessage();
//    void sendDoSAttack();
//    void sendSybilAttack();
//    void sendFalsifiedData();
//
//    // RDO-related methods
//    void initializeRDO();
//    void processRDOUpdate();
//    void sendRDOState();
//    void processRDOStatePacket(Packet *packet);
//    double generateGradient();
//};
//
//#endif // _UEAPP_H_
