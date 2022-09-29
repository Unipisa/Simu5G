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

FLProcess::FLProcess(std::string& name, std::string& flProcessId, std::string& flServiceId, std::string& category,  FLTrainingMode traininigMode)
{
    name_ = name;
    flServiceId_ = flServiceId;
    flProcessId_ = flProcessId;
    category_ = category;
    trainingMode_ = traininigMode;
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

