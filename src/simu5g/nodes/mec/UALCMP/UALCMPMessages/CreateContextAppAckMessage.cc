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

#include "nodes/mec/UALCMP/UALCMPMessages/CreateContextAppAckMessage.h"

namespace simu5g {

CreateContextAppAckMessage::CreateContextAppAckMessage(const char *name, short kind) : CreateContextAppAck(name, kind)
{
}

CreateContextAppAckMessage::CreateContextAppAckMessage(const CreateContextAppAckMessage& other) : CreateContextAppAck(other)
{
    copy(other);
}


CreateContextAppAckMessage& CreateContextAppAckMessage::operator=(const CreateContextAppAckMessage& other)
{
    if (this == &other) return *this;
    CreateContextAppAck::operator=(other);
    copy(other);
    return *this;
}

void CreateContextAppAckMessage::copy(const CreateContextAppAckMessage& other)
{
    this->requestId = other.requestId;
    this->success = other.success;
    this->appInstanceUri = other.appInstanceUri;
    this->appInstanceId = other.appInstanceId;
    this->contextId = other.contextId;
    this->appContext = other.appContext;
}

nlohmann::json CreateContextAppAckMessage::getAppContext() const
{
    return appContext;
}

void CreateContextAppAckMessage::setAppContext(nlohmann::json& appContext)
{
    this->appContext = appContext;
}

} //namespace

