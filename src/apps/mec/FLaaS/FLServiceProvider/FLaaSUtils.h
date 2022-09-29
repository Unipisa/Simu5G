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

#ifndef APPS_MEC_FLAAS_FLSERVICEPROVIDER_FLAASUTILS_H_
#define APPS_MEC_FLAAS_FLSERVICEPROVIDER_FLAASUTILS_H_

#include <string>
#include "inet/networklayer/common/L3Address.h"

enum FLTrainingMode
{
    SYNCHRONOUS, ASYNCHRONOUS, BOTH
};

struct Endpoint {
    inet::L3Address addr;
    int port;
    std::string str() const { return addr.str() + ":" + std::to_string(port);}
};


bool isTrainingMode(std::string& trainingMode);
FLTrainingMode toTrainingMode(std::string& trainingMode);


#endif /* APPS_MEC_FLAAS_FLSERVICEPROVIDER_FLAASUTILS_H_ */
