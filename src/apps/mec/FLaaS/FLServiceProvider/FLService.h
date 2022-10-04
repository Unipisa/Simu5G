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
        std::string flControllerUri_;
        int flServiceIdNumeric_; // variable used internally the framework to differentiate different services
        static int staticFlServiceIdNumeric_;

    public:
        FLService();
        FLService(const char* fileName); // read the FLservice from file from file
        FLService(std::string& name, std::string& flServiceId, std::string description, std::string& category,  FLTrainingMode trainingMode);

        // getters
        std::string getFLServiceName() const {return name_;}
        std::string getFLServiceDescription() const {return description_;}
        std::string getFLServiceId() const {return flServiceId_;}
        std::string getFLControllerUri() const {return flControllerUri_;}
        int getFLServiceIdNumeric() const {return flServiceIdNumeric_;}

//        std::string getFLProcessId() {return flProcessId_;}
        std::string getFLCategory()const {return category_;}
        std::string getFLTrainingModeStr();
//        double getFLCurrentModelAccuracy();
        FLTrainingMode getFLTrainingMode() const {return trainingMode_;}

//        bool isFLServiceActive() {return isActive_;}
//        Endpoint getFLControllerEndpoint() {return flControllerEndpoint_;}

        // setters
        void setFLServiceName(std::string& name) {name_ = name;}
        void setFLServiceDescription(std::string& description) {description_ = description;}
        void setFLServiceId(std::string& serviceId) {flServiceId_ = serviceId;}
        void setFLControllerUri(std::string& uri) {flControllerUri_ = uri;}
//        void setFLProcessId(std::string& processId) {flProcessId_ = processId;}
//        void setFLServiceActive(bool active) {isActive_ = active;}
//        void setFLControllerEndpoint (Endpoint endpoint) {flControllerEndpoint_ = endpoint;}
//        void setFLControllerEndpoint (inet::L3Address& addr, int port) {flControllerEndpoint_.addr = addr; flControllerEndpoint_.port = port;}
        void setFLCategory(std::string& category) {category_ = category;}
//        void setFLTrainingMode(FLTrainingMode mode) {trainingMode_ = mode;}
//        void setFLCurrentModelAccuracy(double accuracy) {currentAccuracy_ = accuracy;}
};



#endif /* APPS_MEC_FLAAS_FLSERVICEPROVIDER_FLSERVICE_H_ */
