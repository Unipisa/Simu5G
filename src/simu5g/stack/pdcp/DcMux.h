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

#ifndef _PDCP_DC_MUX_H_
#define _PDCP_DC_MUX_H_

#include <omnetpp.h>
#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/LteCommon.h"

namespace simu5g {

using namespace omnetpp;

class BearerManagement;

/**
 * @brief Dual Connectivity mux.
 *
 * Routes split-bearer PDCP PDUs between the local PDCP entities and the
 * DcX2Forwarder. Keeps no routing table: each PDU's destination module is
 * looked up in BearerManagement by DrbKey as it arrives.
 */
class DcMux : public cSimpleModule
{
  protected:
    MacNodeId nodeId_;

    BearerManagement *bearerManagement_ = nullptr;

    cGate *dcManagerInGate_ = nullptr;

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(cMessage *msg) override;
};

} // namespace simu5g

#endif
