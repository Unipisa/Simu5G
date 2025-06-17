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

#ifndef __SIMU5G_NRRXSDAPENTITY_H_
#define __SIMU5G_NRRXSDAPENTITY_H_

#include <omnetpp.h>
#include "common/QfiContextManager.h"

using namespace omnetpp;

namespace simu5g {

/**
 * NrRxSdapEntity implements the receive-side SDAP functionality.
 *
 * This module receives PDUs from the PDCP layer, extracts SDAP headers (if present),
 * restores the associated QoS Flow based on QFI, and forwards the resulting SDUs
 * to the upper IP layer.
 *
 * Future extensions may include:
 * - Dynamic QoS Flow handling
 * - Header parsing and verification
 * - Error handling for invalid SDAP PDUs
 * - SDAP Control PDU support (if applicable)
 */
class NrRxSdapEntity : public cSimpleModule
{
  protected:
    QfiContextManager* contextManager = nullptr;
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

} //namespace

#endif
