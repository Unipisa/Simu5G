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


#ifndef STACK_SDAP_COMMON_QFICONTEXTMANAGER_H_
#define STACK_SDAP_COMMON_QFICONTEXTMANAGER_H_

#pragma once

#include <string>
#include <map>
#include "QfiContext.h"

class QfiContextManager
{
  private:
    std::map<int, QfiContext> qfiMap;
    std::map<int, QfiContext> drbMap;

  public:
    void loadFromFile(const std::string& filename);
    const QfiContext* getContextByQfi(int qfi) const;
    const QfiContext* getContextByDrb(int drbIndex) const;
};



#endif /* STACK_SDAP_COMMON_QFICONTEXTMANAGER_H_ */
