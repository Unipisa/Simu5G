

#ifndef _ASSOCIATEID_H_
#define _ASSOCIATEID_H_


#include "nodes/mec/MEPlatform/MeServices/Resources/AttributeBase.h"

#include <string>
#include "nodes/mec/MecCommon.h"
/// <summary>
/// 
/// </summary>
class  AssociateId: public AttributeBase
{
    public:
        AssociateId();
        AssociateId(std::string& type,std::string& value);
        AssociateId(mec::AssociateId& associateId);


        virtual ~AssociateId();

        nlohmann::ordered_json toJson() const override;
        /////////////////////////////////////////////
        /// AssociateId members

        /// <summary>
        /// Numeric value (0-255) corresponding to specified type of identifier
        /// </summary>
        
        void setAssociateId(const mec::AssociateId& associateId);
        

        std::string getType() const;
        void setType(std::string value);
            /// <summary>
        /// Value for the identifier
        /// </summary>
        std::string getValue() const;
        void setValue(std::string value);
        
        MacNodeId getNodeId();

    protected:
        std::string type_;

        std::string value_;

};

#endif /* _ASSOCIATEID_H_ */
