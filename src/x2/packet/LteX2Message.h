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

#ifndef _LTE_LTEX2MESSAGE_H_
#define _LTE_LTEX2MESSAGE_H_

#include "x2/packet/LteX2Message_m.h"
#include "common/LteCommon.h"
#include "x2/packet/X2InformationElement.h"

namespace simu5g {

// add here new X2 message types
enum LteX2MessageType
{
    X2_COMP_MSG, X2_HANDOVER_CONTROL_MSG, X2_HANDOVER_DATA_MSG, X2_DUALCONNECTIVITY_CONTROL_MSG, X2_DUALCONNECTIVITY_DATA_MSG, X2_UNKNOWN_MSG
};

/**
 * @class LteX2Message
 * @brief Base class for X2 messages
 *
 * Class derived from base class contained
 * in msg declaration: adds the Information Elements list
 *
 * Create new X2 Messages by deriving this class
 */
class LteX2Message : public LteX2Message_Base
{
  protected:

    /// type of the X2 message
    LteX2MessageType type_ = X2_UNKNOWN_MSG;

    /// List of X2 IEs
    X2InformationElementsList ieList_;

    /// Size of the X2 message
    int64_t msgLength_ = 0;

  public:

    /**
     * Constructor
     */
    LteX2Message() : LteX2Message_Base()
    {
    }

    /*
     * Copy constructors
     * FIXME Copy constructors do not preserve ownership
     * Here I should iterate on the list and set all ownerships
     */

    LteX2Message& operator=(const LteX2Message& other)
    {
        if (&other == this)
            return *this;
        LteX2Message_Base::operator=(other);
        type_ = other.type_;

        // perform deep-copy of element list
        msgLength_ = other.msgLength_;
        ieList_.clear();
        for (const auto& ie : other.ieList_) {
            ieList_.push_back(ie->dup());
        }

        return *this;
    }

    LteX2Message(const LteX2Message& other) : LteX2Message_Base() {
        operator=(other);
    }

    LteX2Message *dup() const override
    {
        return new LteX2Message(*this);
    }

    ~LteX2Message() override
    {
        while (!ieList_.empty()) {
            delete ieList_.front();
            ieList_.pop_front();
        }
    }

    // getter/setter methods for the type field
    void setType(LteX2MessageType type) { type_ = type; }
    LteX2MessageType getType() const { return type_; }

    /**
     * Getter to access the InformationElement list (e.g. for serialization)
     */
    virtual X2InformationElementsList getIeList() const {
        return ieList_;
    }

    /**
     * pushIe() stores an IE inside the
     * X2 IE list in back position and updates msg length
     *
     * @param ie IE to store
     */
    virtual void pushIe(X2InformationElement *ie)
    {
        ieList_.push_back(ie);
        msgLength_ += ie->getLength();
        // increase the chunk length by length of IE + 1 Byte (required to store the IE type)
        setChunkLength(getChunkLength() + inet::b(8 * (ie->getLength() + sizeof(uint8_t))));
        // EV << "pushIe: pushed an element of length: " << ie->getLength() << " new chunk length: " << getChunkLength() << std::endl;
    }

    /**
     * popIe() pops an IE from the front of
     * the IE list and returns it
     *
     * @return popped IE
     */
    virtual X2InformationElement *popIe()
    {
        X2InformationElement *ie = ieList_.front();
        ieList_.pop_front();
        msgLength_ -= ie->getLength();
        // chunk is immutable during serialization!
        // (chunk length can therefore not be adapted - we only adapt the separate msg_Length_)
        return ie;
    }

    /**
     * hasIe() verifies if there are other
     * IEs inside the IE list
     *
     * @return true if list is not empty, false otherwise
     */
    virtual bool hasIe() const
    {
        return !ieList_.empty();
    }

    int64_t getByteLength() const
    {
        return msgLength_;
    }

    int64_t getBitLength() const
    {
        return getByteLength() * 8;
    }

};

Register_Class(LteX2Message);

} //namespace

#endif

