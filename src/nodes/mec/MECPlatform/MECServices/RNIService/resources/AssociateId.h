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

#ifndef _ASSOCIATEID_H_
#define _ASSOCIATEID_H_

#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"

#include <string>
#include "nodes/mec/utils/MecCommon.h"

namespace simu5g {

/// <summary>
///
/// </summary>
class AssociateId : public AttributeBase
{
  public:
    AssociateId();
    AssociateId(const std::string& type, const std::string& value);
    AssociateId(mec::AssociateId& associateId);


    nlohmann::ordered_json toJson() const override;
    /////////////////////////////////////////////
    /// AssociateId members

    /// <summary>
    /// Numeric value (0-255) corresponding to the specified type of identifier
    /// </summary>

    void setAssociateId(const mec::AssociateId& associateId);

    std::string getType() const;
    void setType(std::string value);
    /// <summary>
    /// Value for the identifier
    /// </summary>
    std::string getValue() const;
    void setValue(std::string value);

  protected:
    std::string type_;
    std::string value_;

};

} //namespace

#endif /* _ASSOCIATEID_H_ */

