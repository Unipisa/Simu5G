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

#ifndef _LTE_LTECONTROLINFO_H_
#define _LTE_LTECONTROLINFO_H_

#include "common/LteControlInfo_m.h"
#include <vector>

namespace simu5g {

class UserTxParams;

/**
 * @class UserControlInfo
 * @brief ControlInfo used in the Lte model
 *
 * This is the LteControlInfo Resource Block Vector
 * and Remote Antennas Set
 *
 */
class UserControlInfo : public UserControlInfo_Base
{
  protected:

    const UserTxParams *userTxParams = nullptr;
    RbMap grantedBlocks;
    /** @brief The movement of the sending host.*/
    //Move senderMovement;
    /** @brief The playground position of the sending host.*/
    inet::Coord senderCoord;

  public:

    /**
     * Constructor: base initialization
     * @param name packet name
     * @param kind packet kind
     */
    UserControlInfo();
    ~UserControlInfo() override;

    /*
     * Operator = : packet copy
     * @param other source packet
     * @return reference to this packet
     */
    UserControlInfo& operator=(const UserControlInfo& other);

    /**
     * Copy constructor: packet copy
     * @param other source packet
     */
    UserControlInfo(const UserControlInfo& other) :
        UserControlInfo_Base()
    {
        operator=(other);
    }

    /**
     * dup() : packet duplicate
     * @return pointer to duplicate packet
     */
    UserControlInfo *dup() const override
    {
        return new UserControlInfo(*this);
    }

    void setUserTxParams(const UserTxParams *arg);

    const UserTxParams *getUserTxParams() const
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

    // struct used to request a feedback computation by nodeB
    FeedbackRequest feedbackReq;
    void setCoord(const inet::Coord& coord);
    inet::Coord getCoord() const;
};

Register_Class(UserControlInfo);

} //namespace

#endif

