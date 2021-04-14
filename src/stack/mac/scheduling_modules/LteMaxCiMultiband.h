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

#ifndef LTEMAXCIMULTIBAND_H_
#define LTEMAXCIMULTIBAND_H_

#include "stack/mac/scheduler/LteScheduler.h"

    typedef SortedDesc<MacCid, unsigned int> ScoreDesc;
    typedef std::priority_queue<ScoreDesc> ScoreList;

class LteMaxCiMultiband : public virtual LteScheduler
{


public:
    LteMaxCiMultiband(){ }
    virtual ~LteMaxCiMultiband() {};

    virtual void prepareSchedule();

    virtual void commitSchedule();
};


#endif /* LTEMAXCIMULTIBAND_H_ */
