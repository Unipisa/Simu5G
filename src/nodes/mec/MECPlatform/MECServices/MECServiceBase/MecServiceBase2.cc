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

#include "nodes/mec/MECPlatform/MECServices/MECServiceBase/MecServiceBase2.h"

#include "nodes/mec/MECPlatformManager/MecPlatformManager.h"

namespace simu5g {

using namespace omnetpp;

void MecServiceBase2::initialize(int stage)
{
    MecServiceBase::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        loadGenerator_ = par("loadGenerator").boolValue();
        if (loadGenerator_) {
            beta_ = par("betaa").doubleValue();
            lambda_ = 1 / beta_;
            numBGApps_ = par("numBGApps").intValue();
            /*
             * check if the system is stable.
             * For M/M/1 --> rho (lambda_/mu) < 1
             * If arrivals come from multiple independent exponential sources:
             * (numBGApps * lambda_) / mu < 1
             */
            double lambdaT = lambda_ * (numBGApps_ + 1);
            rho_ = lambdaT * requestServiceTime_;
            if (rho_ >= 1)
                throw cRuntimeError("M/M/1 system is unstable: rho is %f", rho_);
            EV << "MecServiceBase::initialize - rho: " << rho_ << endl;
        }

        int found = getParentModule()->findSubmodule("serviceRegistry");
        if (found == -1)
            throw cRuntimeError("MecServiceBase::initialize - ServiceRegistry not present!");
        servRegistry_ = check_and_cast<ServiceRegistry *>(getParentModule()->getSubmodule("serviceRegistry"));

        cModule *module = meHost_->getSubmodule("mecPlatformManager");
        if (module != nullptr) {
            EV << "MecServiceBase::initialize - MecPlatformManager found" << endl;
            mecPlatformManager_ = check_and_cast<MecPlatformManager *>(module);
        }

        // get the BSs connected to the mec host
        getConnectedBaseStations();
    }
}

} //namespace

