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

#include "simu5g/stack/sdap/common/QfiContextManager.h"
#include <fstream>
#include <sstream>
#include <cassert>

using namespace simu5g;

QfiContextManager* QfiContextManager::instance = nullptr;

QfiContextManager* QfiContextManager::getInstance() {
    if (!instance)
        instance = new QfiContextManager();
    return instance;
}

void QfiContextManager::registerQfiForCid(MacCid cid, int qfi) {
    cidToQfi_[cid] = qfi;
    qfiToCid_[qfi] = cid;
}

int QfiContextManager::getQfiForCid(MacCid cid) const {
    auto it = cidToQfi_.find(cid);
    return it != cidToQfi_.end() ? it->second : -1;
}

MacCid QfiContextManager::getCidForQfi(int qfi) const {
    auto it = qfiToCid_.find(qfi);
    return it != qfiToCid_.end() ? it->second : MacCid();
}

const QfiContext* QfiContextManager::getContextByQfi(int qfi) const {
    auto it = qfiMap.find(qfi);
    return it != qfiMap.end() ? &it->second : nullptr;
}

const std::map<MacCid, int>& QfiContextManager::getCidToQfiMap() const {
    return cidToQfi_;
}

const std::map<int, QfiContext>& QfiContextManager::getQfiMap() const {
    return qfiMap;
}

// Placeholder for file loader
void QfiContextManager::loadFromFile(const std::string& filename) {
    std::ifstream in(filename);
    if (!in.is_open())
        throw cRuntimeError("QfiContextManager: Failed to open file '%s'", filename.c_str());

    std::string line;

    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        QfiContext ctx;
        std::string gbrStr;

        if (iss >> ctx.qfi >> ctx.drbIndex >> ctx.fiveQi >> gbrStr
                >> ctx.delayBudgetMs >> ctx.packetErrorRate
                >> ctx.priorityLevel >> std::ws)
        {
            ctx.isGbr = (gbrStr == "Yes" || gbrStr == "yes");
            std::getline(iss, ctx.description);
            qfiMap[ctx.qfi] = ctx;
        }
    }

    //TODO enable this (currently fails!)
//    if (in.fail())
//        throw cRuntimeError("QfiContextManager: Failed to read file '%s'", filename.c_str());

    in.close();
}
