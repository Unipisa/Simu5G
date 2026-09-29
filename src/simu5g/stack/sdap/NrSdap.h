//
//                  Simu5G
//
// Authors: Mohamed Seliem (University College Cork), Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __SIMU5G_NRRXSDAPENTITY_H_
#define __SIMU5G_NRRXSDAPENTITY_H_

#include <omnetpp.h>
#include <set>
#include "simu5g/stack/sdap/common/SdapDrbTable.h"
#include "simu5g/stack/sdap/common/ReflectiveQosTable.h"
#include "simu5g/stack/rrc/ConnectionControlBase.h"
#include <inet/common/ModuleRefByPar.h>

using namespace omnetpp;

namespace inet {
class Packet;
}

namespace simu5g {

/**
 * NrSdap implements SDAP functionality.
 *
 * For PDUs received from the PDCP layer, extracts SDAP headers (if present),
 * restores the associated QoS Flow based on QFI, and forwards the resulting SDUs
 * to the upper layer (IP, Ethernet, or application for unstructured sessions).
 *
 * For SDUs received from the upper layer, performs QFI-to-DRB mapping,
 * encapsulates the data with SDAP headers if needed, and forwards the packets
 * to the PDCP layer.
 *
 * A received packet goes up as the protocol the type of its bearer's session
 * implies, which the pushed DRB configuration carries (see DrbDesc::sessionType).
 */
class NrSdap : public cSimpleModule
{
  protected:
    SdapDrbTable drbTable_;
    inet::ModuleRefByPar<ReflectiveQosTable> reflectiveQosTable;
    inet::ModuleRefByPar<ConnectionControlBase> connectionControl_;   // the node's control-plane entry point, the bearer-selection authority

    // The bearers that currently exist on this node, as told by RRC (see
    // bearerEstablished()). A packet mapped to a bearer that is not here needs one
    // established.
    std::set<DrbKey> establishedBearers_;

    bool isUe = true;  // Node role: true for UE, false for gNB
    bool establishBearersOnDemand_ = true;
    bool reflectiveQosOverridesQfi_ = false;   // when a packet has both a classified QFI and a reflective QoS match: true = reflective wins

  protected:
    virtual bool isD2dFlow(inet::Packet *pkt);
    virtual Qfi recoveryQfi(const DrbDesc *drb);
    virtual bool shouldEnableReflectiveQos(Qfi qfi);
    // The protocol a received packet goes up as. On an IP session that is the IP
    // version the datagram carries, which the session type only validates.
    virtual const inet::Protocol *getUpperProtocol(const DrbDesc *ctx, inet::Packet *pkt);
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleUpperPacket(inet::Packet *pkt);
    virtual void handleLowerPacket(inet::Packet *pkt);

  public:
    // Configuration push: RRC installs (or replaces) a bearer's SDAP configuration.
    // SDAP does not author its own configuration; this is the only write path into
    // its DRB table.
    virtual void configureDrb(const DrbDesc& drb);

    // Bearer lifecycle notifications from RRC, which owns the entities. They keep
    // establishedBearers_ current, so "does this DRB's bearer exist?" is answered from
    // this module's own state instead of by inspecting the PDCP layer's registry.
    virtual void bearerEstablished(DrbKey key);
    virtual void bearerReleased(DrbKey key);
};

} //namespace

#endif
