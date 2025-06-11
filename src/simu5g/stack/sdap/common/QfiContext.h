//
//                  Simu5G
//
// Authors: Mohamed Seliem (University College Cork)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
///

#ifndef STACK_SDAP_COMMON_QFICONTEXT_H_
#define STACK_SDAP_COMMON_QFICONTEXT_H_

#pragma once
#include <string>

struct QfiContext {
    int qfi;
    int drbIndex;
    int fiveQi;
    bool isGbr;
    double delayBudgetMs;
    double packetErrorRate;
    int priorityLevel;
    std::string description;
};




#endif /* STACK_SDAP_COMMON_QFICONTEXT_H_ */
