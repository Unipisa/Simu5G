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

#ifndef _ECGI_H_
#define _ECGI_H_

#include <string>

#include "nodes/mec/MECPlatform/MECServices/Resources/AttributeBase.h"
#include "nodes/mec/MECPlatform/MECServices/RNIService/resources/Plmn.h"

namespace simu5g {

class Ecgi : public AttributeBase
{
  protected:
    MacCellId cellId_ = MacCellId(-1);
    Plmn plmn_;

    nlohmann::ordered_json toJsonCell() const; //should this be private?

  public:
    Ecgi();
    Ecgi(MacCellId cellId);
    Ecgi(const mec::Ecgi ecgi);

    Ecgi(MacCellId cellId, Plmn& plmn);


    void setCellId(const MacCellId cellId);
    void setPlmn(const Plmn& plmn);
    void setPlmn(const mec::Plmn plmn);
    void setEcgi(const mec::Ecgi& ecgi);

    MacCellId getCellId() const;
    Plmn getPlmn() const;

    nlohmann::ordered_json toJson() const override;

};

} //namespace

#endif // _ECGI_H_

