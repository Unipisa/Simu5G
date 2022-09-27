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

#ifndef __PLATOON_VEHICLE_INFO_H_
#define __PLATOON_VEHICLE_INFO_H_

#include <ostream>
#include "omnetpp.h"
#include "inet/common/geometry/common/Coord.h"

/*
 * PlatoonVehicleInfo
 *
 * This class stores basic information about a vehicle used by the Platoon Controller.
 *
 * Extend this class if your controller needs additional information
 */
class PlatoonVehicleInfo
{

  protected:

    int mecAppId_;
    int producerAppId_;
    double acceleration_;
    inet::Coord position_;
    inet::Coord lastPosition_;
    double speed_;
    double lastSpeed_;
    double timestamp_;
    double lastTimestamp_;

    inet::L3Address ueAddress_;

  public:
    PlatoonVehicleInfo()
    {
        position_ = inet::Coord::NIL;
        speed_ = 0.0;
        acceleration_ = 0.0;
        timestamp_ = 0.0;
        ueAddress_ = inet::L3Address();
    }
    virtual ~PlatoonVehicleInfo() {}

    friend std::ostream& operator<<(std::ostream& os, const PlatoonVehicleInfo& vehicleInfo)
    {
        os << "pos" << vehicleInfo.position_;
        os << ", ";
        os << "speed(" << vehicleInfo.speed_ << "mps)";
        return os;
    }

    void setPosition(inet::Coord position) { position_ = position; }
    void setPositionX(double position) { position_.x = position; }
    void setPositionY(double position) { position_.y = position; }
    void setPositionZ(double position) { position_.z = position; }

    inet::Coord getPosition() const { return position_; }

    void setLastPosition(inet::Coord position) { lastPosition_ = position; }
    inet::Coord getLastPosition() const { return lastPosition_; }


    void setSpeed(double speed) { speed_ = speed; }
    double getSpeed() const { return speed_; }

    void setAcceleration(double acceleration) { acceleration_ = acceleration; }
    double getAcceleration() const { return acceleration_; }

    void setLastSpeed(double speed) { lastSpeed_ = speed; }
    double getLastSpeed() const { return lastSpeed_; }


    void setMecAppId(int mecAppId) { mecAppId_ = mecAppId; }
    int getMecAppId() const { return mecAppId_; }

    void setProducerAppId(int producerAppId) { producerAppId_ = producerAppId; }
    int getProducerAppId() const { return producerAppId_; }


    void setTimestamp(double timestamp) { timestamp_ = timestamp; }
    double getTimestamp() const { return timestamp_; }

    void setLastTimestamp(double timestamp) { lastTimestamp_ = timestamp; }
    double getLastTimestamp() const { return lastTimestamp_; }



    void setUeAddress(inet::L3Address address) { ueAddress_ = address; }
    inet::L3Address getUeAddress() const { return ueAddress_; }
};

#endif
