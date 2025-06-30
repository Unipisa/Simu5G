// @author Alessandro Noferi
//

#ifndef __UTILS_H
#define __UTILS_H

#include <stdlib.h>
#include <string>
#include <string.h>
#include <vector>

#ifdef _WIN32
# include <io.h>
# include <stdio.h>
#else // ifdef _WIN32
# include <unistd.h>
#endif // ifdef _WIN32

#include <inet/networklayer/common/L3Address.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>

namespace simu5g {

using namespace omnetpp;

namespace utils {

std::vector<std::string> splitString(const std::string& str, std::string delim);

class cModule_LessId
{
  public:
    bool operator()(const cModule *left, const cModule *right) const;
};

} // namespace utils

} // namespace simu5g

#endif // ifndef __UTILS_H

