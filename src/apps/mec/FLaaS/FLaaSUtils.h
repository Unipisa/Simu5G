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
#include "inet/common/TimeTag_m.h"


enum FLTrainingMode
{
    SYNCHRONOUS, ASYNCHRONOUS, BOTH, ONE_SHOT
};

enum MsgType
{
    START_ROUND, START_ROUND_ACK, TRAIN_GLOBAL_MODEL, TRAINED_MODEL, LEARNER_DEPLOYMENT, DATA_MSG
};


typedef struct  {
    inet::L3Address addr;
    int port;
//    std::string str() const { return addr.str() + ":" + std::to_string(port);}
}Endpoint;


typedef struct
{
    omnetpp::simtime_t timestamp;
    double dimension;
}MLModel;


typedef struct
{
    int id;
    Endpoint endpoint;
} AvailableLearner;



bool isTrainingMode(std::string& trainingMode);
FLTrainingMode toTrainingMode(std::string& trainingMode);


#endif /* APPS_MEC_FLAAS_FLSERVICEPROVIDER_FLAASUTILS_H_ */
