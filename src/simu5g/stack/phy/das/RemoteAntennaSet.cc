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

#include "stack/phy/das/RemoteAntennaSet.h"

namespace simu5g {

void RemoteAntennaSet::addRemoteAntenna(double ruX, double ruY, double ruPow)
{
    RemoteAntenna ru;
    inet::Coord ruPos = inet::Coord(ruX, ruY);
    ru.ruPosition_ = ruPos;
    ru.txPower_ = ruPow;
    remoteAntennaSet_.push_back(ru);
}

inet::Coord RemoteAntennaSet::getAntennaCoord(unsigned int remote)
{
    if (remote >= remoteAntennaSet_.size())
        return inet::Coord(0, 0);
    return remoteAntennaSet_[remote].ruPosition_;
}

double RemoteAntennaSet::getAntennaTxPower(unsigned int remote)
{
    if (remote >= remoteAntennaSet_.size())
        return 0.0;
    return remoteAntennaSet_[remote].txPower_;
}

unsigned int RemoteAntennaSet::getAntennaSetSize()
{
    return remoteAntennaSet_.size();
}

std::ostream& operator<<(std::ostream& stream, const RemoteAntennaSet *ruSet)
{
    if (ruSet == nullptr)
        return stream << "Empty set";
    for (unsigned int i = 0; i < ruSet->remoteAntennaSet_.size(); i++) {
        stream << "RU" << i << " : " << "Pos = (" <<
            ruSet->remoteAntennaSet_[i].ruPosition_.x << "," <<
            ruSet->remoteAntennaSet_[i].ruPosition_.y << ") ; txPow = " <<
            ruSet->remoteAntennaSet_[i].txPower_ << " :: ";
    }
    return stream;
}

} //namespace

