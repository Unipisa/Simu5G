//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//


//
// RnisTestAppPacket used by UeRnisTestApp and MecRnisTestApp
//

import nodes.mec.MECPlatform.MECPackets;

cplusplus {{
#include "nodes/mec/utils/httpUtils/json.hpp"
}}

class nlohmann::json 
{ 
    @existingClass;
}

namespace simu5g;

class RnisTestAppPacket extends MECPacket
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
    nlohmann::json l2meas;
}
