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

#include "apps/mec/FLaaS/FLaaSUtils.h"

bool isTrainingMode(std::string& trainMode)
{
    if(trainMode.compare("SYNCHRONOUS") == 0)
        return true;
    else if(trainMode.compare("ASYNCHRONOUS") == 0)
        return true;
    else if(trainMode.compare("BOTH") == 0)
        return true;
    else if(trainMode.compare("ONE_SHOT") == 0)
            return true;

    else
        return false;
}

// N.B trainingMode param must be checked before using this function!
FLTrainingMode toTrainingMode(std::string& trainingMode)
{
    if(trainingMode.compare("SYNCHRONOUS") == 0)
        return SYNCHRONOUS;
    else if(trainingMode.compare("ASYNCHRONOUS") == 0)
        return ASYNCHRONOUS;
    else if(trainingMode.compare("BOTH") == 0)
        return BOTH;
    else
        return ONE_SHOT;
}

