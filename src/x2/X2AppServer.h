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

#ifndef __X2APPSERVER_H_
#define __X2APPSERVER_H_

#include <inet/applications/sctpapp/SctpServer.h>
#include "common/LteCommon.h"
#include "common/binder/Binder.h"


/**
 * Implements the X2AppServer simple module. See the NED file for more info.
 */
class X2AppServer : public inet::SctpServer
{
        // reference to the gate
    omnetpp::cGate* x2ManagerIn_;

    protected:
        virtual void initialize(int stage) override;
        virtual void handleMessage(omnetpp::cMessage *msg) override;
        void handleTimer(omnetpp::cMessage *msg);
        void generateAndSend(inet::Packet* pkt);
};

#endif


