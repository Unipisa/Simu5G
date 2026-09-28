//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/d2d/rrc/ConnectionControlUeD2D.h"

#include "simu5g/stack/d2d/binder/D2dBinder.h"
#include "simu5g/stack/d2d/phy/PhyUeD2D.h"
#include "simu5g/stack/ip2nic/HandoverPacketHolderUe.h"
#include "simu5g/stack/phy/feedback/LteDlFeedbackGenerator.h"
#include "simu5g/common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(ConnectionControlUeD2D);

UeCapabilities ConnectionControlUeD2D::getCapabilities() const
{
    UeCapabilities capabilities;
    capabilities.d2d = dynamic_cast<PhyUeD2D *>(phy_) != nullptr;
    return capabilities;
}

void ConnectionControlUeD2D::bearerReleased(DrbKey bearer)
{
    Enter_Method_Silent("bearerReleased");
    if (getNodeTypeById(bearer.getNodeId()) == NODEB)
        return;   // an infrastructure bearer: its base station releases the id

    // A dual-stack UE may have established the bearer under either of its own ids, so
    // offer it back to both pools -- releasing an id that is not in use there is a no-op
    D2dBinder *d2dBinder = D2dBinder::getInstance(this);
    MacNodeId otherId = hasOtherLeg() ? otherConnectionControl_->getNodeId() : NODEID_NONE;
    for (MacNodeId ownId : {nodeId_, otherId}) {
        if (ownId == NODEID_NONE)
            continue;
        auto pair = std::minmax(ownId, bearer.getNodeId());
        if (d2dBinder->sidelinkDrbIdPool({pair.first, pair.second}).erase(bearer.getDrbId()) != 0)
            EV << "ConnectionControlUeD2D::bearerReleased - DRB " << bearer.getDrbId() << " of the sidelink node pair ("
               << pair.first << ", " << pair.second << ") is free again" << endl;
    }
}

} //namespace
