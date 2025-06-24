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
#include "simu5g/common/LteCommon.h"

namespace simu5g {

class QfiContextManager {
  private:
    std::map<int, QfiContext> qfiMap;            // QFI → QoS context
    std::map<MacCid, int> cidToQfi_;             // CID → QFI
    std::map<int, MacCid> qfiToCid_;             // QFI → CID
    static QfiContextManager* instance;

  public:
    static QfiContextManager* getInstance();

    void loadFromFile(const std::string& filename);

    // Register a single QFI ↔ CID pair
    void registerQfiForCid(MacCid cid, int qfi);

    // Accessors
    const QfiContext* getContextByQfi(int qfi) const;
    int getQfiForCid(MacCid cid) const;
    MacCid getCidForQfi(int qfi) const;
    const std::map<int, QfiContext>& getQfiMap() const;
    const std::map<MacCid, int>& getCidToQfiMap() const;
};

} // namespace simu5g

#endif /* STACK_SDAP_COMMON_QFICONTEXTMANAGER_H_ */
