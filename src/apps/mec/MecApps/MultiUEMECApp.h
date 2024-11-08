//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#pragma once

#include <omnetpp.h>
#include "inet/networklayer/common/L3Address.h"
#include "apps/mec/MecApps/MecAppBase.h"

using namespace omnetpp;

namespace simu5g {

struct UE_MEC_CLIENT {
    inet::L3Address address;
    int port;
};

class MultiUEMECApp : public MecAppBase
{
protected:
    virtual void handleSelfMessage(omnetpp::cMessage *msg) {};
    virtual void handleServiceMessage(int connId) {};
    virtual void handleMp1Message(int connId) {};
    virtual void handleHttpMessage(int connId) {};
    virtual void handleUeMessage(omnetpp::cMessage *msg) {};
    virtual void established(int connId) {};
public:
    /**
     * Method used to notify the MEC app that a new UE would like to associate with that
     * Every MEC app inheriting from this class, must implement this method
     */
    virtual void addNewUE(struct UE_MEC_CLIENT ueData);
};

}
