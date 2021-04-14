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

#include "stack/mac/packet/LteSchedulingGrant_m.h"
#include "common/LteCommon.h"
#include "stack/mac/amc/UserTxParams.h"

class UserTxParams;

class LteSchedulingGrant : public LteSchedulingGrant_Base
{
private:
    void copy(const LteSchedulingGrant& other) {
        if (other.userTxParams != nullptr)
        {
            const UserTxParams* txParams = check_and_cast<const UserTxParams*>(other.userTxParams);
            userTxParams = txParams->dup();
        }
        else
        {
            userTxParams = nullptr;
        }
        grantedBlocks = other.grantedBlocks;
        grantedCwBytes = other.grantedCwBytes;
        direction_ = other.direction_;
    }
  protected:

    const UserTxParams* userTxParams;
    RbMap grantedBlocks;
    std::vector<unsigned int> grantedCwBytes;
    Direction direction_;

  public:

    LteSchedulingGrant() :
        LteSchedulingGrant_Base()
    {
        userTxParams = nullptr;
        grantedCwBytes.resize(MAX_CODEWORDS);
    }

    ~LteSchedulingGrant()
    {
        if (userTxParams != nullptr)
        {
            delete userTxParams;
            userTxParams = nullptr;
        }
    }

    LteSchedulingGrant(const LteSchedulingGrant& other) : LteSchedulingGrant_Base(other)
    {
        copy(other);
    }

    LteSchedulingGrant& operator=(const LteSchedulingGrant& other)
    {
        if (this == &other) return *this;
        LteSchedulingGrant_Base::operator=(other);
        copy(other);
        return *this;
    }

    virtual LteSchedulingGrant *dup() const override
    {
        return new LteSchedulingGrant(*this);
    }

    void setUserTxParams(const UserTxParams* arg)
    {
        if(userTxParams){
            delete userTxParams;
        }
        userTxParams = arg;
    }

    const UserTxParams* getUserTxParams() const
    {
        return userTxParams;
    }

    const unsigned int getBlocks(Remote antenna, Band b) const
        {
        return grantedBlocks.at(antenna).at(b);
    }

    void setBlocks(Remote antenna, Band b, const unsigned int blocks)
    {
        grantedBlocks[antenna][b] = blocks;
    }

    const RbMap& getGrantedBlocks() const
    {
        return grantedBlocks;
    }

    void setGrantedBlocks(const RbMap& rbMap)
    {
        grantedBlocks = rbMap;
    }

    virtual void setGrantedCwBytesArraySize(size_t size) override
    {
        grantedCwBytes.resize(size);
    }
    virtual size_t getGrantedCwBytesArraySize() const override
    {
        return grantedCwBytes.size();
    }
    virtual unsigned int getGrantedCwBytes(size_t k) const override
    {
        return grantedCwBytes.at(k);
    }
    virtual void setGrantedCwBytes(size_t k, unsigned int grantedCwBytes_var) override
    {
        grantedCwBytes[k] = grantedCwBytes_var;
    }

    virtual void insertGrantedCwBytes(unsigned int grantedCwBytes) override {
        throw cRuntimeError("insertGrantedCwBytes not implemented");
    }
    virtual void insertGrantedCwBytes(size_t k, unsigned int grantedCwBytes) override {
        throw cRuntimeError("insertGrantedCwBytes not implemented");
    }
    virtual void eraseGrantedCwBytes(size_t k) override {
        throw cRuntimeError("eraseGrantedCwBytes not implemented");
    }

    void setDirection(Direction dir)
    {
        direction_ = dir;
    }
    Direction getDirection() const
    {
        return direction_;
    }
};
