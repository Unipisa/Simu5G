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

#ifndef APPS_MEC_FLAAS_FLProcessPROVIDER_FLProcess_H_
#define APPS_MEC_FLAAS_FLProcessPROVIDER_FLProcess_H_

#include "inet/networklayer/common/L3Address.h"
#include "apps/mec/FLaaS/FLServiceProvider/FLaaSUtils.h"

class FLProcess
{
    private:
        std::string name_;
        std::string flServiceId_;
        std::string flProcessId_;
        std::string category_;
        FLTrainingMode trainingMode_;
        bool isActive_;
        Endpoint flControllerEndpoint_;
        double currentAccuracy_;
        int mecAppId_;

    public:
        FLProcess() {};
        FLProcess(const std::string& name, const std::string& flProcessId, const std::string& flServiceId, const std::string& category, const FLTrainingMode traininigMode, const int mecAppId);

        // getters
        std::string getFLProcessName() const {return name_;}
        std::string getFLServiceId() const {return flServiceId_;}
        std::string getFLProcessId() const {return flProcessId_;}
        std::string getFLCategory() const {return category_;}
        std::string getFLTrainingModeStr();
        double getFLCurrentModelAccuracy();
        FLTrainingMode getFLTrainingMode() const {return trainingMode_;}

        bool isFLProcessActive() const {return isActive_;}
        Endpoint getFLControllerEndpoint() const {return flControllerEndpoint_;}

        // setters
        void setFLProcessName(std::string& name) {name_ = name;}
        void setFLProcessId(std::string& serviceId) {flServiceId_ = serviceId;}
        void setFlProcessId(std::string& processId) {flProcessId_ = processId;}
        void setFLProcessActive(bool active) {isActive_ = active;}
        void setFlControllerEndpoint (Endpoint endpoint) {flControllerEndpoint_ = endpoint;}
        void setFlControllerEndpoint (inet::L3Address& addr, int port) {flControllerEndpoint_.addr = addr; flControllerEndpoint_.port = port;}
        void setFlCategory(std::string& category) {category_ = category;}
        void setFLTrainingMode(FLTrainingMode mode) {trainingMode_ = mode;}
        void setFLCurrentModelAccuracy(double accuracy) {currentAccuracy_ = accuracy;}
};

#endif /* APPS_MEC_FLAAS_FLProcessPROVIDER_FLProcess_H_ */
