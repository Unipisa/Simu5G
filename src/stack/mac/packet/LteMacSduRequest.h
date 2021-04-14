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

#ifndef _LTE_LTEMACSDUREQUEST_H_
#define _LTE_LTEMACSDUREQUEST_H_

#include "stack/mac/packet/LteMacSduRequest_m.h"
#include "common/LteCommon.h"

/**
 * @class LteMacSduRequest
 * @brief Lte MAC SDU Request message
 *
 * Class derived from base class contained
 * in msg declaration: adds UE ID and Logical Connection ID
 */
class LteMacSduRequest : public LteMacSduRequest_Base
{
  private:
    void copy(const LteMacSduRequest& other) {
        ueId_ = other.ueId_;
        lcid_ = other.lcid_;
    }
  protected:
    /// ID of the destination UE associated with the request
    MacNodeId ueId_;

    /// Logical Connection identifier associated with the request
    LogicalCid lcid_;

  public:

    /**
     * Constructor
     */
    LteMacSduRequest() :
        LteMacSduRequest_Base()
    {
    }

    /**
     * Destructor
     */
    virtual ~LteMacSduRequest()
    {
    }

    LteMacSduRequest(const LteMacSduRequest& other) :
        LteMacSduRequest_Base(other)
    {
        copy(other);
    }

    LteMacSduRequest& operator=(const LteMacSduRequest& other)
    {
        if (&other == this)
            return *this;
        LteMacSduRequest_Base::operator=(other);
        copy(other);
        return *this;
    }

    virtual LteMacSduRequest *dup() const
    {
        return new LteMacSduRequest(*this);
    }
    MacNodeId getUeId() { return ueId_; }
    void setUeId(MacNodeId ueId) { ueId_ = ueId; }

    LogicalCid getLcid() { return lcid_; }
    void setLcid(LogicalCid lcid) { lcid_ = lcid; }
};

Register_Class(LteMacSduRequest);

#endif

