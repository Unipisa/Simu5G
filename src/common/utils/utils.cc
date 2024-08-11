#include "common/utils/utils.h"

namespace simu5g {

using namespace omnetpp;

namespace utils {

std::vector<std::string> splitString(const std::string& str, std::string delim) {
    size_t last = 0;
    size_t next = 0;
    std::vector<std::string> splitted;
    std::string line;
    if (str.size() == 0) return splitted;
    while ((next = str.find(delim, last)) != std::string::npos) {
        line = str.substr(last, next - last);
        if (line.size() != 0) splitted.push_back(line);
        last = next + delim.size();
    }

    // add the last token only if the delim is at the end of the str
    // otherwise, an empty string would be added
    if (last != str.size())
        splitted.push_back(str.substr(last, next - last)); // last token
    return splitted;
}

bool cModule_LessId::operator()(const cModule *left, const cModule *right) const
{
    return (left != nullptr ? left->getId() : -1) < (right != nullptr ? right->getId() : -1);
}

} // namespace utils

} //namespace

