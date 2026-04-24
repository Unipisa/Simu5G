//
// Copyright (C) 2023 Mario Franke <research@m-franke.net>
//
// Documentation for these modules is at http://sat.car2x.org/
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//
//
// This module has been modified to work with Simu5G. Moreover, it now uses
// GeographicLib instead of PROJ library. Original name was SGP4Mobility.
//

package simu5g.mobility.satellite;

import inet.mobility.base.MovingMobilityBase;

//
// Mobility module for LEO satellites based on the SGP4 propagation model.
//
simple LeoSatMobility extends MovingMobilityBase
{
    parameters:
        @class(LeoSatMobility);
        @display("i=block/cogwheel");
        string satelliteName = default("");
        string tleLine1 = default("");
        string tleLine2 = default("");
        string wallClockSimStartTimeUtc = default("");
        @signal[mobilityStateChanged](type=inet::MobilityBase);
}
