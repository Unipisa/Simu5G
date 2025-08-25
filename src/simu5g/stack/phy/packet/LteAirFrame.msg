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

import simu5g.stack.phy.packet.AirFrame;

cplusplus {{
#include "simu5g/common/LteCommon.h"  // for RemoteUnitPhyDataVector
}}
 
namespace simu5g;

struct RemoteUnitPhyDataVector
{
    @existingClass;
    @opaque;
    //@toString(.str());
}

//
// This is LteAirFrame.
//
packet LteAirFrame extends AirFrame {
    @customize(true);
    RemoteUnitPhyDataVector remoteUnitPhyDataVector;
}

cplusplus(LteAirFrame) {{
    void addRemoteUnitPhyDataVector(const RemoteUnitPhyData& data) { remoteUnitPhyDataVector.push_back(data); }
}}
