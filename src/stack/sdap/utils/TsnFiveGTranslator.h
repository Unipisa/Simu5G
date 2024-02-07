/*
 * TsnFiveGTranslator.h
 *
 *  Created on: Apr 24, 2023
 *      Author: devika
 */
#ifndef STACK_SDAP_TSNFIVEGTRANSLATOR_H_
#define STACK_SDAP_TSNFIVEGTRANSLATOR_H_
#include <iostream>
#include <omnetpp.h>
#include "inet/linklayer/ieee8021q/Ieee8021qTagHeader_m.h"

using namespace inet;


static inet::Packet convertFiveGPacketToTsnPacket(inet::Packet *pkt){
    int typeOrLength = 2048;
    int qfi = -1;
    auto header = makeShared<Ieee8021qTagEpdHeader>();

    return *pkt;
}
#endif /* STACK_SDAP_TSNFIVEGTRANSLATOR_H_ */
