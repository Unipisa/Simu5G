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

package simu5g.mobility.satellite.SatelliteInserter;

simple SatelliteInserter
{
    parameters:
        @display("i=block/control");
        string satelliteModuleType = default("simu5g.nodes.Leo600Satellite");
        string satelliteModuleDefaultName = default("Satellite");
        bool ignoreUnknownSatellites = default(true); // Satellites whose name does not start with a constellation name listed in examples/space_veins/SatelliteExampleScenario.ned will be ignored
        string pathToTleFile = default("satellite_tle.txt");
        string wall_clock_sim_start_time_utc; // yyyy-mm-dd-hh-mm-ss

}
