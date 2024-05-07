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

#include "QosHandler.h"

Define_Module(QosHandlerUE);
Define_Module(QosHandlerGNB);
Define_Module(QosHandlerUPF);

void QosHandler::initialize()
{
    // TODO - Generated method body
}

void QosHandler::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
}

void QosHandlerUE::initialize(int stage) {

    if (stage == 0) {
        nodeType = UE;
        initQfiParams();
        globalData = check_and_cast<GlobalData*>(getSimulation()->findModuleByPath("globalData"));

    }

}

void QosHandlerUE::handleMessage(cMessage *msg) {
    // TODO - Generated method body

    //std::cout << "QosHandlerUE::handleMessage start at " << simTime().dbl() << std::endl;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//GNB
void QosHandlerGNB::initialize(int stage) {

    if (stage == 0) {
        nodeType = GNODEB;
        initQfiParams();
        globalData = check_and_cast<GlobalData*>(getSimulation()->findModuleByPath("globalData"));

    }
}

void QosHandlerGNB::handleMessage(cMessage *msg) {
    // TODO - Generated method body

    //std::cout << "QosHandlerGNB::handleMessage start at " << simTime().dbl() << std::endl;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//UPF
void QosHandlerUPF::initialize(int stage) {
    if (stage == 0) {
        nodeType = UNKNOWN_NODE_TYPE;
        initQfiParams();
        globalData = check_and_cast<GlobalData*>(getSimulation()->findModuleByPath("globalData"));

    }
}

void QosHandlerUPF::handleMessage(cMessage *msg) {
    // TODO - Generated method body

    //std::cout << "QosHandlerUPF::handleMessage start at " << simTime().dbl() << std::endl;
}
