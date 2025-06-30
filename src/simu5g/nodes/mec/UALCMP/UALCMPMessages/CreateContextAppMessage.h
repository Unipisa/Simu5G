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

#ifndef NODES_MEC_LCMPROXY_LCMPROXYMESSAGES_CREATECONTEXTAPPMESSAGE_H_
#define NODES_MEC_LCMPROXY_LCMPROXYMESSAGES_CREATECONTEXTAPPMESSAGE_H_

#include "nodes/mec/UALCMP/UALCMPMessages/UALCMPMessages_m.h"
#include "nodes/mec/utils/httpUtils/json.hpp"

namespace simu5g {

class CreateContextAppMessage : public CreateContextApp
{
  private:
    nlohmann::json appContext;

    void copy(const CreateContextAppMessage& other);

  public:
    CreateContextAppMessage(const char *name = nullptr, short kind = 0);
    CreateContextAppMessage(const CreateContextAppMessage& other);
    CreateContextAppMessage& operator=(const CreateContextAppMessage& other);
    CreateContextAppMessage *dup() const override { return new CreateContextAppMessage(*this); }

    virtual nlohmann::json getAppContext() const;
    virtual void setAppContext(nlohmann::json& appContext);

};

} //namespace

#endif /* NODES_MEC_LCMPROXY_LCMPROXYMESSAGES_CREATECONTEXTAPPMESSAGE_H_ */

