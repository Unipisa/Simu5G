/*
 * CreateContextAppAckMessageMessage.cc
 *
 *  Created on: May 7, 2021
 *      Author: linofex
 */



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




