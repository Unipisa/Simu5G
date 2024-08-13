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

#include "common/LteControlInfo.h"
#include "stack/mac/amc/UserTxParams.h"

namespace simu5g {

using namespace inet;

UserControlInfo::~UserControlInfo()
{
    if (userTxParams != nullptr) {
        delete userTxParams;
        userTxParams = nullptr;
    }
}

UserControlInfo::UserControlInfo() :
    UserControlInfo_Base()
{
}

UserControlInfo& UserControlInfo::operator=(const UserControlInfo& other)
{
    if (&other == this)
        return *this;

    if (other.userTxParams != nullptr) {
        const UserTxParams *txParams = check_and_cast<const UserTxParams *>(other.userTxParams);
        this->userTxParams = txParams->dup();
    }
    else {
        this->userTxParams = nullptr;
    }
    this->grantedBlocks = other.grantedBlocks;
    this->senderCoord = other.senderCoord;
    this->feedbackReq = other.feedbackReq;
    UserControlInfo_Base::operator=(other);
    return *this;
}

void UserControlInfo::setCoord(const inet::Coord& coord)
{
    senderCoord = coord;
}

void UserControlInfo::setUserTxParams(const UserTxParams *newParams)
{
    if (userTxParams != nullptr) {
        delete userTxParams;
    }
    userTxParams = newParams;
}

inet::Coord UserControlInfo::getCoord() const
{
    return senderCoord;
}

} //namespace

