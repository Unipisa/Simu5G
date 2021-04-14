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

#ifndef __MEWARNINGALERTAPP_H_
#define __MEWARNINGALERTAPP_H_

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

//MEWarningAlertPacket
#include "nodes/mec/MEPlatform/MEAppPacket_Types.h"
#include "apps/mec/warningAlert/packets/WarningAlertPacket_m.h"

using namespace std;
using namespace omnetpp;
/**
 * See MEWarningAlertApp.ned
 */
class MEWarningAlertApp : public omnetpp::cSimpleModule
{
    char* ueSimbolicAddress;
    char* meHostSimbolicAddress;
    inet::L3Address destAddress_;
    int size_;

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        void finish();

        void handleInfoUEWarningAlertApp(cMessage* msg);
        void handleInfoMEWarningAlertApp(cMessage* msg);
};

#endif
