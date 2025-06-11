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

#include "QfiContextManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

void QfiContextManager::loadFromFile(const std::string& filename)
{
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        throw std::runtime_error("Cannot open QFI config file: " + filename);
    }

    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        QfiContext context;
        std::string gbrStr;

        if (!(iss >> context.qfi >> context.drbIndex >> context.fiveQi >> gbrStr
                  >> context.delayBudgetMs >> context.packetErrorRate >> context.priorityLevel)) {
            continue; // skip invalid lines
        }

        context.isGbr = (gbrStr == "Yes" || gbrStr == "yes" || gbrStr == "1");

        // (Optional: parse description if present)
        std::getline(iss, context.description);
        if (!context.description.empty() && context.description[0] == ' ')
            context.description.erase(0, 1); // remove leading space

        qfiMap[context.qfi] = context;
        drbMap[context.drbIndex] = context;
    }

    infile.close();
}

const QfiContext* QfiContextManager::getContextByQfi(int qfi) const
{
    auto it = qfiMap.find(qfi);
    if (it != qfiMap.end())
        return &(it->second);
    else
        return nullptr;
}

const QfiContext* QfiContextManager::getContextByDrb(int drbIndex) const
{
    auto it = drbMap.find(drbIndex);
    if (it != drbMap.end())
        return &(it->second);
    else
        return nullptr;
}
