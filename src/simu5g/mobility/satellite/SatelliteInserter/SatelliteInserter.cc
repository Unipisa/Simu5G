//
// Copyright (C) 2006-2017 Christoph Sommer <sommer@ccs-labs.org>
// Copyright (C) 2023 Mario Franke <research@m-franke.net>
//
// Documentation for these modules is at http://sat.car2x.org/
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include <fstream>
#include <sstream>
#include <regex>

#include "simu5g/mobility/satellite/SatelliteInserter/SatelliteInserter.h"
#include "simu5g/mobility/satellite/LeoSatMobility.h"
#include "veins/base/utils/FindModule.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(SatelliteInserter);

void SatelliteInserter::initialize(int stage)
{
    switch (stage)
    {
        case 0:
            pathToTleFile = par("pathToTleFile").stringValue();
            satelliteModuleType = par("satelliteModuleType").stringValue();
            satelliteModuleDefaultName = par("satelliteModuleDefaultName").stringValue();
            wall_clock_sim_start_time_utc = par("wall_clock_sim_start_time_utc").stringValue();
            ignoreUnknownSatellites = par("ignoreUnknownSatellites").boolValue();
            break;
        case 98:
            parseTleFile(pathToTleFile);
            break;
    }
}

void SatelliteInserter::parseTleFile(std::string path)
{
    std::ifstream tleFile(path);
    std::string tleLine;
    int tleLineNumber = 0;
    TLE tle;
    while (std::getline(tleFile, tleLine))
    {
        switch (tleLineNumber)
        {
            case 0:
                tle.set_satellite_name(tleLine);
                break;
            case 1:
                tle.set_tle_line1(tleLine);
                break;
            case 2:
                tle.set_tle_line2(tleLine);
                EV_DEBUG << "Initializing satellite: " << std::endl << tle << std::endl;
                instantiateSatellite(tle);
                break;
        }
        if (tleLineNumber < 2) {
            tleLineNumber++;
        }else{
            tleLineNumber = 0;
        }
    }
    // Check completeness of all satellites
    if (tleLineNumber != 0) {
        throw cRuntimeError("Incomplete satellite TLE");
    }
}

std::pair<std::string, unsigned int> SatelliteInserter::getConstellationAndSatNum(TLE tle)
{
    std::string satelliteName = tle.get_satellite_name();
    std::vector<std::string> delimiters = {"-", " "};
    size_t posConstellation = std::string::npos;
    std::string delimiter;
    for (auto iter = delimiters.begin(); iter != delimiters.end(); ++iter) {
        posConstellation = satelliteName.find(*iter);
        if(satelliteName[posConstellation - 1] == '[' && satelliteName[posConstellation + 1] == ']') {
            // Some satellite names have a suffix "[-]". This occurence of "-" should not be treated as a delimiter.
            posConstellation = std::string::npos;
            continue;
        }
        if (posConstellation != std::string::npos) {
            // we found the correct delimiter, we can stop searching
            delimiter = *iter;
            break;
        }
    }
    std::string constellation = satelliteName.substr(0, posConstellation);
    satelliteName.erase(0, posConstellation + delimiter.length());
    // Delete non-numeric characters
    std::string regex = R"([\D])";
    satelliteName = std::regex_replace(satelliteName, std::regex(regex), "");
    // Delete leading zeros
    satelliteName.erase(0, satelliteName.find_first_not_of('0'));
    unsigned int satNum;
    try {
        satNum = std::stoul(satelliteName, nullptr, 0);
    }
    catch (const std::invalid_argument& ia) {
        // if satNum is not a number, use the catalog number 
        satNum = std::stoul(tle.get_tle_line1().substr(2, 5));
    }
    if (satNum == 0) {
        EV_DEBUG << "stop" << std::endl;
    }
    return std::pair<std::string, unsigned int>(constellation, satNum);
}

void SatelliteInserter::createSatellite(TLE tle, unsigned int satNum, unsigned int vectorSize, std::string constellation) {
    cModule* parentmod = getParentModule();
    if (!parentmod) throw cRuntimeError("Parent Module not found");

    cModuleType* satType = cModuleType::get(satelliteModuleType.c_str());
    if (!satType) throw cRuntimeError("Module Type \"%s\" not found", satelliteModuleType.c_str());

    #if OMNETPP_BUILDNUM >= 1525
        parentmod->setSubmoduleVectorSize(("leo" + constellation).c_str(), vectorSize);
        cModule* mod = satType->create(("leo" + constellation).c_str(), parentmod, satNum);
    #else
        // TODO: this trashes the vectsize member of the cModule, although nobody seems to use it
        cModule* mod = satType->create(("leo" + constellation).c_str(), parentmod, vectorSize, satNum);
    #endif

    mod->finalizeParameters();
    mod->buildInside();
    auto leoSatMobilityModules = veins::getSubmodulesOfType<LeoSatMobility>(mod);
    for (auto mobility : leoSatMobilityModules) {
        mobility->preInitialize(tle, wall_clock_sim_start_time_utc);
    }
    mod->scheduleStart(simTime());
    mod->callInitialize();
}

void SatelliteInserter::instantiateSatellite(TLE tle)
{
    // Determine satellite constellation
    std::pair<std::string, unsigned int> satellite = getConstellationAndSatNum(tle);

    // Create satellite
    if (satellite.first == "STARLINK") {
        starlinkVectorSize = std::max(starlinkVectorSize, satellite.second + 1);
        createSatellite(tle, satellite.second, starlinkVectorSize, satellite.first);
        return;
    }
    if (satellite.first == "IRIDIUM") {
        iridiumVectorSize = std::max(iridiumVectorSize, satellite.second + 1);
        createSatellite(tle, satellite.second, iridiumVectorSize, satellite.first);
        return;
    }
    if (satellite.first == "ORBCOMM") {
        orbcommVectorSize = std::max(orbcommVectorSize, satellite.second + 1);
        createSatellite(tle, satellite.second, orbcommVectorSize, satellite.first);
        return;
    }
    if (satellite.first == "SPACEBEE") {
        spacebeeVectorSize = std::max(spacebeeVectorSize, satellite.second + 1);
        createSatellite(tle, satellite.second, spacebeeVectorSize, satellite.first);
        return;
    }
    if (satellite.first == "SPACEBEENZ") {
        spacebeenzVectorSize = std::max(spacebeenzVectorSize, satellite.second + 1);
        createSatellite(tle, satellite.second, spacebeenzVectorSize, satellite.first);
        return;
    }
    if (satellite.first == "ONEWEB") {
        onewebVectorSize = std::max(onewebVectorSize, satellite.second + 1);
        createSatellite(tle, satellite.second, onewebVectorSize, satellite.first);
        return;
    }
    if (satellite.first == "GLOBALSTAR") {
        globalstarVectorSize = std::max(globalstarVectorSize, satellite.second + 1);
        createSatellite(tle, satellite.second, globalstarVectorSize, satellite.first);
        return;
    }
    if (ignoreUnknownSatellites) {
        // Satellites whose name does not start with a constellation name will be discarded.
        return;
    }
    // Use catalog number for satellites that could not be assign to a constellation
    satelliteVectorSize = std::max(satelliteVectorSize, (unsigned int)std::stoul(tle.get_tle_line1().substr(2, 5)) + 1);
    createSatellite(tle, std::stoul(tle.get_tle_line1().substr(2, 5)), satelliteVectorSize, satelliteModuleDefaultName);
}

void SatelliteInserter::finish()
{
}

} // namespace simu5g
