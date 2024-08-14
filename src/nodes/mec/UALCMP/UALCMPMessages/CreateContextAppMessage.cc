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

#include "nodes/mec/UALCMP/UALCMPMessages/CreateContextAppMessage.h"

namespace simu5g {

CreateContextAppMessage::CreateContextAppMessage(const char *name, short kind) : CreateContextApp(name, kind)
{
}

CreateContextAppMessage::CreateContextAppMessage(const CreateContextAppMessage& other) : CreateContextApp(other)
{
    copy(other);
}


CreateContextAppMessage& CreateContextAppMessage::operator=(const CreateContextAppMessage& other)
{
    if (this == &other) return *this;
    CreateContextApp::operator=(other);
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

} //namespace

