/*
 * CreateContextAppMessageMessage.cc
 *
 *  Created on: May 7, 2021
 *      Author: linofex
 */



#include "nodes/mec/LCMProxy/LCMProxyMessages/CreateContextAppMessage.h"

CreateContextAppMessage::CreateContextAppMessage(const char *name, short kind) : ::CreateContextApp(name, kind)
{
}

CreateContextAppMessage::CreateContextAppMessage(const CreateContextAppMessage& other) : ::CreateContextApp(other)
{
    copy(other);
}

CreateContextAppMessage::~CreateContextAppMessage()
{
}

CreateContextAppMessage& CreateContextAppMessage::operator=(const CreateContextAppMessage& other)
{
    if (this == &other) return *this;
    ::CreateContextApp::operator=(other);
    copy(other);
    return *this;
}

void CreateContextAppMessage::copy(const CreateContextAppMessage& other)
{
    this->requestId = other.requestId;
    this->onboarded = other.onboarded;
    this->appPackagePath = other.appPackagePath;
    this->appDId = other.appDId;
    this->appContext = other.appContext;
}

nlohmann::json CreateContextAppMessage::getAppContext() const
{
    return appContext;
}

void CreateContextAppMessage::setAppContext(nlohmann::json& appContext)
{
    this->appContext = appContext;
}

