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

#include "simu5g/stack/ip2nic/NtnIp2Nic.h"

#include <inet/common/ModuleAccess.h>

namespace simu5g {

using namespace omnetpp;

Define_Module(NtnIp2Nic);

void NtnIp2Nic::initialize(int stage)
{
    Ip2Nic::initialize(stage);

    if (stage == INITSTAGE_SIMU5G_NODE_RELATIONSHIPS && nodeType_ == NODEB) {
        ntnAssociationRegistered_ = registerNtnAssociation(false);

        // Deferred to the first event even when the association is already in place. A dynamic
        // satellite may still register after this stage, and the path geometry cannot be read
        // here in any case: no radio holds a valid position until inet::INITSTAGE_SINGLE_MOBILITY,
        // which runs after every Simu5G stage.
        scheduleAt(SIMTIME_ZERO, new cMessage("initNtnAssociation"));
    }
}

void NtnIp2Nic::handleMessage(cMessage *msg)
{
    if (msg->isName("initNtnAssociation")) {
        delete msg;
        if (!ntnAssociationRegistered_)
            ntnAssociationRegistered_ = registerNtnAssociation(true);
        reportNtnPath();
        return;
    }

    Ip2Nic::handleMessage(msg);
}

void NtnIp2Nic::reportNtnPath()
{
    const GnbNtnAssociation *association = binder_->getGnbNtnAssociation(nodeId_);
    if (association == nullptr)
        return;

    // Evaluated before the stream expression: the Binder logs its own derivation the first time a
    // cell is queried, which would otherwise land in the middle of this line.
    double cellRoundTripDelay = binder_->getNtnCellRoundTripDelay(nodeId_).dbl();

    EV_INFO << "NtnIp2Nic::reportNtnPath - gNodeB " << nodeId_ << " serves through gateway "
            << association->ntnGatewayId << " and satellite " << association->satelliteId
            << ": worst-case cell round-trip delay[" << cellRoundTripDelay * 1000.0 << "ms]" << endl;

    // The per-UE delay is the one that tracks satellite motion, so it is worth having beside the
    // cell bound: the bound must always be the larger of the two.
    for (MacNodeId ueId : binder_->getDeployedUes(nodeId_)) {
        double ueRoundTripDelay = binder_->getNtnRoundTripDelay(nodeId_, ueId).dbl();
        EV_INFO << "NtnIp2Nic::reportNtnPath - UE " << ueId << " round-trip delay["
                << ueRoundTripDelay * 1000.0 << "ms]" << endl;
    }
}

bool NtnIp2Nic::registerNtnAssociation(bool throwOnMissing)
{
    cModule *bs = inet::getContainingNode(this);
    MacNodeId ntnGatewayId = MacNodeId(bs->par("ntnGatewayId").intValue());
    MacNodeId satelliteId = MacNodeId(bs->par("satelliteId").intValue());
    double minElevation = bs->par("ntnMinElevation").doubleValue();
    double minSatelliteAltitude = bs->par("ntnMinSatelliteAltitude").doubleValue();

    if (!binder_->nodeExists(ntnGatewayId) || !binder_->nodeExists(satelliteId)) {
        if (throwOnMissing)
            throw cRuntimeError("NtnIp2Nic::registerNtnAssociation - NTN gateway %hu or satellite %hu is not registered",
                    num(ntnGatewayId), num(satelliteId));
        return false;
    }

    binder_->setGnbNtnAssociation(nodeId_, ntnGatewayId, satelliteId, minElevation, minSatelliteAltitude, true);
    return true;
}

} // namespace simu5g
