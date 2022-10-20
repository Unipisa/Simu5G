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


#include "apps/mec/FLaaS/FLServiceProvider/FLProcess.h"

FLProcess::FLProcess(const std::string& name, const std::string& flProcessId, const std::string& flServiceId, const std::string& category, const FLTrainingMode traininigMode, const int mecAppId)
{
    name_ = name;
    flServiceId_ = flServiceId;
    flProcessId_ = flProcessId;
    category_ = category;
    trainingMode_ = traininigMode;
    mecAppId_ = mecAppId;
    isActive_ = true;
}

std::string FLProcess::getFLTrainingModeStr()
{
    if(trainingMode_ == SYNCHRONOUS)
        return "SYNCHRONOUS";
    else if(trainingMode_ == ASYNCHRONOUS)
        return "ASYNCHRONOUS";
    else if(trainingMode_ == BOTH)
            return "BOTH";
    else
        throw omnetpp::cRuntimeError("FLProcess::getFLTrainingModeStr() - FL Training Mode not recognized ");
}

