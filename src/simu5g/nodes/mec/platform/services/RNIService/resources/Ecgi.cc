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

#include "Ecgi.h"

namespace simu5g {

Ecgi::Ecgi()
{
}

Ecgi::Ecgi(MacCellId cellId)
{
    setCellId(cellId);
}

Ecgi::Ecgi(MacCellId cellId, Plmn& plmn) : plmn_(plmn)
{
    setCellId(cellId);
}

Ecgi::Ecgi(const mec::Ecgi ecgi)
{
    setEcgi(ecgi);
}

void Ecgi::setCellId(MacCellId cellId)
{
    cellId_ = cellId;
}

void Ecgi::setEcgi(const mec::Ecgi& ecgi)
{
    setCellId(ecgi.cellId);
    plmn_.setMnc(ecgi.plmn.mnc);
    plmn_.setMcc(ecgi.plmn.mcc);
}

void Ecgi::setPlmn(const Plmn& plmn)
{
    plmn_ = plmn;
}

void Ecgi::setPlmn(const mec::Plmn plmn)
{
    plmn_.setMcc(plmn.mcc);
    plmn_.setMnc(plmn.mnc);
}

MacCellId Ecgi::getCellId() const
{
    return cellId_;
}

Plmn Ecgi::getPlmn() const
{
    return plmn_;
}

nlohmann::ordered_json Ecgi::toJson() const
{
    nlohmann::ordered_json val;
    val["cellId"] = cellId_;
    val["plmn"] = plmn_.toJson();

    return val;
}

} //namespace

