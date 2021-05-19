/*
 * CategoryRef.h
 *
 *  Created on: May 14, 2021
 *      Author: linofex
 */

#ifndef NODES_MEC_MEPLATFORM_SERVICEREGISTRY_RESOURCES_CATEGORYREF_H_
#define NODES_MEC_MEPLATFORM_SERVICEREGISTRY_RESOURCES_CATEGORYREF_H_


#include "nodes/mec/MEPlatform/MeServices/Resources/AttributeBase.h"


class CategoryRef : public AttributeBase
{
    protected:
        std::string href_;
        std::string id_;
        std::string name_;
        std::string version_;

    public:
        CategoryRef(){};
        CategoryRef(const std::string& href, const std::string& id, const std::string& name, const std::string& version);
        ~CategoryRef(){};
        nlohmann::ordered_json toJson() const;
};




#endif /* NODES_MEC_MEPLATFORM_SERVICEREGISTRY_RESOURCES_CATEGORYREF_H_ */
