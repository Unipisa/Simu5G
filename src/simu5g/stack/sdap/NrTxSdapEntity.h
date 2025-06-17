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

#ifndef __SIMU5G_NRTXSDAPENTITY_H_
#define __SIMU5G_NRTXSDAPENTITY_H_

#include <omnetpp.h>
#include "common/QfiContextManager.h"  // Include the new context manager

using namespace omnetpp;

namespace simu5g {

/**
 * NrTxSdapEntity implements the transmit-side SDAP functionality.
 *
 * This module receives SDUs from the IP layer, performs QFI-to-DRB mapping,
 * encapsulates the data with SDAP headers if needed, and forwards the packets
 * to the PDCP layer.
 *
 * Future extensions may include:
 * - Dynamic QoS Flow handling
 * - Header compression
 * - Error handling
 * - SDAP Control PDU support (if applicable)
 */
class NrTxSdapEntity : public cSimpleModule
{
  protected:
    QfiContextManager* contextManager = nullptr;
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

} //namespace

#endif
