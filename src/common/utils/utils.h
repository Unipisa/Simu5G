// @author Alessandro Noferi
//

#ifndef __UTILS_H
#define __UTILS_H

#include <vector>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <string>

#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3Address.h"

#ifdef _WIN32
# include <io.h>
# include <stdio.h>
#else // ifdef _WIN32
# include <unistd.h>
#endif // ifdef _WIN32

namespace simu5g {

namespace utils {

std::vector<std::string> splitString(const std::string& str, std::string delim);

class cModule_LessId
{
  public:
    bool operator()(const omnetpp::cModule *left, const omnetpp::cModule *right) const;
};

} // namespace utils

} // namespace simu5g

#endif // ifndef __UTILS_H

