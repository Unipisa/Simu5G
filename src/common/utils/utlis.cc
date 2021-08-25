

#include "common/utils/utils.h"

namespace lte{
namespace utils {

    std::vector<std::string> splitString(const std::string& str, std::string delim){
        size_t last = 0;
        size_t next = 0;
        std::vector<std::string> splitted;
        std::string line;
        if(str.size() == 0) return splitted;
        while ((next = str.find(delim, last)) != std::string::npos) {
                line = str.substr(last, next-last);
                if(line.size() != 0) splitted.push_back(line);
                last = next + delim.size();
        }

        // add the last token only if  the delim is at the end of the str
        // otherwise an empty string would be added
        if(last != str.size())
            splitted.push_back(str.substr(last, next-last)); // last token
        return splitted;
    }

    std::string getPacketPayload(omnetpp::cMessage *msg){
//        inet::RawPacket *request = check_and_cast<inet::RawPacket *>(msg);
//        if (request == 0) throw cRuntimeError("UEWarningAlertApp_rest::handleMessage - \tFATAL! Error when casting to MEAppPacket");
//        int pktSize = request->getByteLength();
//        std::string packet(request->getByteArray().getDataPtr(), pktSize);

        std::string packet("ciaso");
              return packet;
    }

//    inet::RawPacket* createUDPPacket(const std::string& payload){
//       inet::RawPacket *pck  = new inet::RawPacket("udpPacket");
//       pck->setDataFromBuffer(payload.c_str(), payload.size());
//       pck->setByteLength(payload.size());
//       return pck;
//    }


}
}

