//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef NODES_MEC_LCMPROXY_LCMPROXYMESSAGES_CREATECONTEXTAPPMESSAGE_H_
#define NODES_MEC_LCMPROXY_LCMPROXYMESSAGES_CREATECONTEXTAPPMESSAGE_H_

#include "nodes/mec/LCMProxy/LCMProxyMessages/LcmProxyMessages_m.h"
#include "nodes/mec/utils/httpUtils/json.hpp"


class CreateContextAppMessage : public CreateContextApp
{
    private:
        nlohmann::json appContext;

        void copy(const CreateContextAppMessage& other);

    public:
        CreateContextAppMessage(const char *name=nullptr, short kind=0);
        CreateContextAppMessage(const CreateContextAppMessage& other);
        virtual ~CreateContextAppMessage();
        CreateContextAppMessage& operator=(const CreateContextAppMessage& other);
        virtual CreateContextAppMessage *dup() const override {return new CreateContextAppMessage(*this);}


        virtual nlohmann::json getAppContext() const;
        virtual void setAppContext(nlohmann::json& appContext);

};



#endif /* NODES_MEC_LCMPROXY_LCMPROXYMESSAGES_CREATECONTEXTAPPMESSAGE_H_ */
