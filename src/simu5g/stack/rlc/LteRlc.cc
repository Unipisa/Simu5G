//
//                  Simu5G
//
// Authors: Mohamed Seliem (University College Cork)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include <inet/common/ModuleAccess.h>
#include "simu5g/stack/rlc/LteRlc.h"

namespace simu5g {

Define_Module(LteRlc);

void LteRlc::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        drbId_ = par("drbIndex").intValue();  // default DRB ID if not vectorized
    }

    if (stage == inet::INITSTAGE_NETWORK_CONFIGURATION) {
        if (inet::getContainingNode(this)->hasPar("nrMacNodeId"))
            nodeId_ = MacNodeId(inet::getContainingNode(this)->par("nrMacNodeId").intValue());
        else
            nodeId_ = MacNodeId(inet::getContainingNode(this)->par("macNodeId").intValue());

        Binder* binder = check_and_cast<Binder*>(getModuleByPath("binder"));
        binder->registerRlcInstance(nodeId_, drbId_, UM, this);

        EV << "LteRlc: Registered RLC instance: NodeId=" << nodeId_
           << ", DRB=" << drbId_ << ", Module=" << getFullPath() << endl;
    }
}

} // namespace
