//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "nodes/mec/LCMProxy/LCMProxyMessages/CreateContextAppAckMessage.h"

CreateContextAppAckMessage::CreateContextAppAckMessage(const char *name, short kind) : ::CreateContextAppAck(name, kind)
{
}

CreateContextAppAckMessage::CreateContextAppAckMessage(const CreateContextAppAckMessage& other) : ::CreateContextAppAck(other)
{
    copy(other);
}

CreateContextAppAckMessage::~CreateContextAppAckMessage()
{
}

CreateContextAppAckMessage& CreateContextAppAckMessage::operator=(const CreateContextAppAckMessage& other)
{
    if (this == &other) return *this;
    ::CreateContextAppAck::operator=(other);
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




