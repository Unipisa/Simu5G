/*
 * CreateContextAppAckMessage.h
 *
 *  Created on: May 7, 2021
 *      Author: linofex
 */

#ifndef NODES_MEC_LCMPROXY_LCMPROXYMESSAGES_CREATECONTEXTAPPACKMESSAGE_H_
#define NODES_MEC_LCMPROXY_LCMPROXYMESSAGES_CREATECONTEXTAPPACKMESSAGE_H_

#include "nodes/mec/LCMProxy/LCMProxyMessages/LcmProxyMessages_m.h"
#include "nodes/mec/MEPlatform/MeServices/httpUtils/json.hpp"


class CreateContextAppAckMessage : public CreateContextAppAck
{
    private:
        nlohmann::json appContext;

        void copy(const CreateContextAppAckMessage& other);

    public:
        CreateContextAppAckMessage(const char *name=nullptr, short kind=0);
        CreateContextAppAckMessage(const CreateContextAppAckMessage& other);
        virtual ~CreateContextAppAckMessage();
        CreateContextAppAckMessage& operator=(const CreateContextAppAckMessage& other);
        virtual CreateContextAppAckMessage *dup() const override {return new CreateContextAppAckMessage(*this);}


        virtual nlohmann::json getAppContext() const;
        virtual void setAppContext(nlohmann::json& appContext);

};



#endif /* NODES_MEC_LCMPROXY_LCMPROXYMESSAGES_CREATECONTEXTAPPACKMESSAGE_H_ */
