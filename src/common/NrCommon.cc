/*
 * NrCommon.cc
 *
 *  Created on: Apr 23, 2023
 *      Author: devika
 */


#include "common/NrCommon.h"

//using namespace inet;

NRQosCharacteristics * NRQosCharacteristics::instance = nullptr;
ResourceType convertStringToResourceType(std::string type){

    //std::cout << "NRBinder::convertStringToResourceType start at " << simTime().dbl() << std::endl;

    if(type == "GBR")
        return GBR;
    else if (type == "NGBR")
        return NGBR;
    else if(type == "DCGBR")
        return DCGBR;
    else
        return UNKNOWN_RES;
}
