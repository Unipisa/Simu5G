//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

import simu5g.mec.platform.MecPackets;

cplusplus {{
#include "simu5g/mec/utils/httpUtils/json.hpp"
}}

class nlohmann::json
{
    @existingClass;
}

namespace simu5g;

//
// RnisTestAppPacket used by UeRnisTestApp and MecRnisTestApp
//
class RnisTestAppPacket extends MecPacket
{
}

class RnisTestAppStartPacket extends RnisTestAppPacket
{
    double period;   // determines the RNIS querying period
}

class RnisTestAppStopPacket extends RnisTestAppPacket
{
}

class RnisTestAppInfoPacket extends RnisTestAppPacket
{
    nlohmann::json l2meas @nopack;
}
