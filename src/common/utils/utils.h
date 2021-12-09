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


namespace lte{
namespace utils {


std::vector<std::string> splitString(const std::string& str, std::string delim);
std::string getPacketPayload(omnetpp::cMessage *msg);
//inet::RawPacket* createUDPPacket(const std::string& payload);

} // namespace utils
} // namespace lte


#endif // ifndef __INET_HTTPUTILS_H
