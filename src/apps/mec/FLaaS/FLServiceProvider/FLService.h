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

#ifndef APPS_MEC_FLAAS_FLSERVICEPROVIDER_FLSERVICE_H_
#define APPS_MEC_FLAAS_FLSERVICEPROVIDER_FLSERVICE_H_

#include "inet/networklayer/common/L3Address.h"
#include "apps/mec/FLaaS/FLServiceProvider/FLaaSUtils.h"

class FLService
{
    private:
        std::string name_;
        std::string flServiceId_;
        std::string description_;
        std::string category_;
        FLTrainingMode trainingMode_;
        bool isActive_;
        Endpoint flControllerEndpoint_;
        double currentAccuracy_;

        /*
         * TODO
         * use a structure to save the last N (or all) model updates.
         * For now it is not necessary
         */


    public:
        FLService() {};
        FLService(std::string& name, std::string& flServiceId, std::string description, std::string& category,  FLTrainingMode trainingMode);

        // getters
        std::string getFlServiceName() {return name_;}
        std::string getFlServiceDescription() {return description_;}
        std::string getFlServiceId() {return flServiceId_;}
        std::string getFLCategory() {return category_;}
        std::string getFLTrainingModeStr();
        double getFLCurrentModelAccuracy();
        FLTrainingMode getFLTrainingMode() {return trainingMode_;}

        bool isFlServiceActive() {return isActive_;}
        Endpoint getFLControllerEndpoint() {return flControllerEndpoint_;}

        // setters
        void setFlServiceName(std::string& name) {name_ = name;}
        void setFlServiceDescription(std::string& description) {description_ = description;}
        void setFlServiceId(std::string& serviceId) {flServiceId_ = serviceId;}
        void setFlServiceActive(bool active) {isActive_ = active;}
        void setFlControllerEndpoint (Endpoint endpoint) {flControllerEndpoint_ = endpoint;}
        void setFlControllerEndpoint (inet::L3Address& addr, int port) {flControllerEndpoint_.addr = addr; flControllerEndpoint_.port = port;}
        void setFlCategory(std::string& category) {category_ = category;}
        void setFLTrainingMode(FLTrainingMode mode) {trainingMode_ = mode;}
        void setFLCurrentModelAccuracy(double accuracy) {currentAccuracy_ = accuracy;}
};



#endif /* APPS_MEC_FLAAS_FLSERVICEPROVIDER_FLSERVICE_H_ */
