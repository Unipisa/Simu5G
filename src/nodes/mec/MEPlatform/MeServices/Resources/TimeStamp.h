#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_


#include "nodes/mec/MEPlatform/MeServices/Resources/AttributeBase.h"


/// <summary>
/// 
/// </summary>
class  TimeStamp: public AttributeBase
{

    protected:
    int32_t seconds_;
    int32_t nanoSeconds_;
    bool valid_;

    public:
        TimeStamp();
        TimeStamp(bool valid);
        
        virtual ~TimeStamp();


        bool isValid() const;
        void setValid(bool valid);

        nlohmann::ordered_json toJson() const override;
       
        /// <summary>
        /// The seconds part of the time. Time is defined as Unix-time since January 1, 1970, 00:00:00 UTC
        /// </summary>
        int32_t getSeconds() const;
        void setSeconds(int32_t value);
        void setSeconds();
        /// <summary>
        /// The nanoseconds part of the time. Time is defined as Unix-time since January 1, 1970, 00:00:00 UTC
        /// </summary>
        int32_t getNanoSeconds() const;
        void setNanoSeconds(int32_t value);
    

};

#endif /* _TIMESTAMP_H_ */
