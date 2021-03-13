/*
 * LocationInfo.h
 *
 *  Created on: Dec 5, 2020
 *      Author: linofex
 */

#ifndef CORENETWORK_NODES_MEC_MEPLATFORM_MESERVICES_LOCATIONSERVICE_RESOURCES_LOCATIONINFO_H_
#define CORENETWORK_NODES_MEC_MEPLATFORM_MESERVICES_LOCATIONSERVICE_RESOURCES_LOCATIONINFO_H_

#include "nodes/mec/MEPlatform/MeServices/Resources/AttributeBase.h"
#include "inet/common/geometry/common/Coord.h"

class LocationInfo : public AttributeBase
{
    public:
        LocationInfo();
        LocationInfo(const inet::Coord& coordinates, const inet::Coord& speed);
        virtual ~LocationInfo();
        nlohmann::ordered_json toJson() const override;

    private:
        inet::Coord coordinates_;
        inet::Coord speed_;
};


#endif /* CORENETWORK_NODES_MEC_MEPLATFORM_MESERVICES_LOCATIONSERVICE_RESOURCES_LOCATIONINFO_H_ */
