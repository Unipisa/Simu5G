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

#include "apps/mecRequestResponseApp/MecRequestApp.h"

namespace simu5g {

simsignal_t MecRequestApp::requestSizeSignal_ = registerSignal("requestSize");
simsignal_t MecRequestApp::requestRTTSignal_ = registerSignal("requestRTT");
simsignal_t MecRequestApp::recvResponseSnoSignal_ = registerSignal("recvResponseSno");

Define_Module(MecRequestApp);


MecRequestApp::~MecRequestApp() {
    cancelAndDelete(selfSender_);
}

void MecRequestApp::initialize(int stage)
{
    EV << "MecRequestApp::initialize - stage " << stage << endl;
    cSimpleModule::initialize(stage);
    // avoid multiple initializations
    if (stage != inet::INITSTAGE_APPLICATION_LAYER)
        return;

    //retrieve parameters
    period_ = par("period");
    localPort_ = par("localPort");
    destPort_ = par("destPort");
    sourceSymbolicAddress_ = getContainingNode(this)->getFullName();
    const char *destSymbolicAddress = par("destAddress").stringValue();
    destAddress_ = inet::L3AddressResolver().resolve(destSymbolicAddress);

    //binding socket UDP
    socket.setOutputGate(gate("socketOut"));
    socket.bind(localPort_);

    int tos = par("tos");
    if (tos != -1)
        socket.setTos(tos);

    sno_ = 0;
    nrPhy_.reference(this, "phyModule", true);
    bsId_ = nrPhy_->getMasterId();
    appId_ = par("appId");

    enableMigration_ = par("enableMigration").boolValue();

    //initializing the auto-scheduling messages
    selfSender_ = new cMessage("selfSender");

    //starting MecRequestApp
    simtime_t startTime = par("startTime");
    EV << "MecRequestApp::initialize - sending first packet in " << startTime << " seconds " << endl;
    scheduleAt(simTime() + startTime, selfSender_);
}

void MecRequestApp::handleMessage(cMessage *msg)
{
    EV << "MecRequestApp::handleMessage" << endl;
    // Sender Side
    if (msg->isSelfMessage()) {
        if (!strcmp(msg->getName(), "selfSender")) {
            // update BS ID
            if (enableMigration_ && bsId_ != nrPhy_->getMasterId()) {
                MigrationTimer *migrationTimer = new MigrationTimer("migrationTimer");
                migrationTimer->setOldAppId(num(bsId_));
                migrationTimer->setNewAppId(num(nrPhy_->getMasterId()));

                double migrationTime = uniform(20, 30);
                scheduleAt(simTime() + migrationTime, migrationTimer);
            }

            bsId_ = nrPhy_->getMasterId();
            sendRequest();

            //rescheduling
            scheduleAt(simTime() + period_, selfSender_);
        }
        else if (!strcmp(msg->getName(), "migrationTimer")) {
            MigrationTimer *migrationTimer = check_and_cast<MigrationTimer *>(msg);
            appId_ = migrationTimer->getNewAppId();
            delete migrationTimer;
        }
        else
            throw cRuntimeError("MecRequestApp::handleMessage - WARNING: Unrecognized self message");
    }
    // Receiver Side
    else {
        recvResponse(msg);
        delete msg;
    }
}

void MecRequestApp::finish()
{
    // ensuring there is no selfStop_ scheduled!
    if (selfSender_->isScheduled())
        cancelEvent(selfSender_);
}

/*
 * -----------------------------------------------Sender Side------------------------------------------
 */
void MecRequestApp::sendRequest()
{
    EV << "MecRequestApp::sendRequest - Sending request number " << sno_ << " to " << destAddress_.str() << endl;

    // read next packet size
    int size = par("packetSize");
    emit(requestSizeSignal_, (long)size);

    inet::Packet *packet = new inet::Packet("MecRequestResponsePacket");
    auto reqPkt = makeShared<MecRequestResponsePacket>();
    reqPkt->setSno(sno_++);
    reqPkt->setAppId(appId_);
    reqPkt->setBsId(bsId_);
    reqPkt->setReqTimestamp(simTime().dbl());
    reqPkt->setChunkLength(B(size));
    reqPkt->setSrcAddress(sourceSymbolicAddress_.c_str());
    reqPkt->setSrcPort(localPort_);
    reqPkt->setDestAddress(destAddress_.str().c_str());
    reqPkt->setDestPort(destPort_);
    packet->insertAtBack(reqPkt);
    socket.sendTo(packet, destAddress_, destPort_);
}

/*
 * ---------------------------------------------Receiver Side------------------------------------------
 */
void MecRequestApp::recvResponse(cMessage *msg)
{
    inet::Packet *packet = check_and_cast<inet::Packet *>(msg);
    packet->removeControlInfo();

    auto respPkt = packet->popAtFront<MecRequestResponsePacket>();

    unsigned int sno = respPkt->getSno();
    double reqTimestamp = respPkt->getReqTimestamp();

    double rtt = simTime().dbl() - reqTimestamp;

    EV << "MecRequestApp::recvResponse - sno[" << sno << "] rtt[" << rtt << "]" << endl;

    // emit statistics
    emit(requestRTTSignal_, rtt);
    emit(recvResponseSnoSignal_, (long)sno);
}

} //namespace

