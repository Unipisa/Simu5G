/*
 * LocalManagerStatus.h
 *
 *  Created on: Oct 12, 2022
 *      Author: alessandro
 */

#ifndef APPS_MEC_FLAAS_FLCONTROLLER_LOCALMANAGERSTATUS_H_
#define APPS_MEC_FLAAS_FLCONTROLLER_LOCALMANAGERSTATUS_H_

#include "apps/mec/FLaaS/FLaaSUtils.h"

class LocalManagerStatus
{
    private:
        int id_;
        int learnerId_;
        Endpoint lmEndpoint_;
        Endpoint learnerEndpoint_;


    public:
        LocalManagerStatus(){}
        LocalManagerStatus(int id, int learnerId, Endpoint& lmEndpoint, Endpoint& learnerEndpoint ){ id_ = id; learnerId_ = learnerId, lmEndpoint_ = lmEndpoint; learnerEndpoint_ = learnerEndpoint;}
        int getLocalManagerId()const {return id_;}
        int getLearnerId()const {return learnerId_;}

        Endpoint getLocalManagerEndpoint() const {return lmEndpoint_;}
        Endpoint getLearnerEndpoint() const {return learnerEndpoint_;}


        void setLocalManagerId(int id ) {id_ = id;}
        void setLearnerId(int id ) {learnerId_ = id;}

        void setLocalManagerEndpoint(Endpoint& lmEndpoint) {lmEndpoint_ = lmEndpoint;}
        void setLearnerEndpoint(Endpoint& learnerEndpoint) {learnerEndpoint_ = learnerEndpoint;}

};


#endif /* APPS_MEC_FLAAS_FLCONTROLLER_LOCALMANAGERSTATUS_H_ */
