//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#pragma once

#include "inet/networklayer/configurator/ipv4/Ipv4NetworkConfigurator.h"
#include "inet/networklayer/common/NetworkInterface.h"
#include "common/LteCommon.h"



using namespace omnetpp;
using namespace inet;


class CellularNetworkConfigurator : public inet::Ipv4NetworkConfigurator {
public:
    virtual ~CellularNetworkConfigurator() = default;


protected:
    virtual void initialize(int stage) override;
    virtual bool isWirelessInterface(NetworkInterface  *netIf) override;

    std::vector<std::pair<std::string, int>> interfaceComparator;
};

