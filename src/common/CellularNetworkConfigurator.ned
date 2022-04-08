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
// Note: This extension would better be in the L3NetworkConfiguratorBase class
//       within the inet framework. We have opened a corrensponding pull request
//       (see https://github.com/inet-framework/inet/pull/753) - if it is merged,
//       the CellularNetworkConfigurator can be removed completely for newer INET 
//       versions (>4.3.7).)

package simu5g.common;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;

// 
// See Ipv4NetworkConfigurator for extensive documentation
//
// This module only allows setting of interface names which should be 
// treated as wireless interfaces during network configuration. 
// Ipv4NetworkConfigurator only accepts 'wlan' as hard coded interface name.
// This module uses a configurable list of interface names.
simple CellularNetworkConfigurator extends Ipv4NetworkConfigurator {
    
    parameters:
    @class(CellularNetworkConfigurator);
    string wirelessInterfaceNames = default("wlan,cellular"); // comma seperated no spaces.
}
