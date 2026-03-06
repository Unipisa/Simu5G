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
#include "simu5g/stack/sdap/common/DrbTable.h"
#include "simu5g/stack/sdap/common/ReflectiveQosTable.h"
#include "simu5g/common/binder/Binder.h"
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
 * Supports all 3GPP PDU session types (IPv4, IPv6, Ethernet, Unstructured)
 * via the pduSessionType field in drbConfig.
 */
class NrSdap : public cSimpleModule
{
  protected:
    DrbTable drbTable_;
    inet::ModuleRefByPar<ReflectiveQosTable> reflectiveQosTable;
    inet::ModuleRefByPar<Binder> binder_;
    bool isUe = true;  // Node role: true for UE, false for gNB

    // Tracks (drbId, destId) pairs for which connections have been established
    std::set<std::pair<DrbId, MacNodeId>> establishedConnections_;

  protected:
    bool requiresSdapHeader(const DrbConfig *drb);
    bool shouldEnableReflectiveQos(Qfi qfi);
    const inet::Protocol *getUpperProtocol(const DrbConfig *ctx);
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleUpperPacket(inet::Packet *pkt);
    virtual void handleLowerPacket(inet::Packet *pkt);

};

} //namespace

#endif
