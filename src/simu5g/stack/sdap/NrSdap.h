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
#include "simu5g/stack/sdap/common/QfiContextManager.h"
#include "simu5g/stack/sdap/common/ReflectiveQosTable.h"
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
 * to the upper IP layer.

 * For SDUs received from the IP layer, performs QFI-to-DRB mapping,
 * encapsulates the data with SDAP headers if needed, and forwards the packets
 * to the PDCP layer.
 *
 * Future extensions may include:
 * - Dynamic QoS Flow handling
 * - Header parsing and verification
 * - Error handling for invalid SDAP PDUs
 * - SDAP Control PDU support (if applicable)
 */
class NrSdap : public cSimpleModule
{
  protected:
    QfiContextManager* contextManager = nullptr;
    inet::ModuleRefByPar<ReflectiveQosTable> reflectiveQosTable;
    bool isUe = true;  // Node role: true for UE, false for gNB

  protected:
    bool requiresSdapHeader(int drbIndex);
    bool shouldEnableReflectiveQos(int qfi);
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void handleUpperPacket(inet::Packet *pkt);
    virtual void handleLowerPacket(inet::Packet *pkt);
};

} //namespace

#endif
