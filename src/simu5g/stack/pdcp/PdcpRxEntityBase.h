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

#ifndef _PDCP_RX_ENTITY_BASE_H_
#define _PDCP_RX_ENTITY_BASE_H_

#include <omnetpp.h>
#include <inet/common/InitStages.h>

namespace inet { class Packet; }

namespace simu5g {

/**
 * @class PdcpRxEntityBase
 * @brief Abstract base class for all RX PDCP entities.
 *
 * Defines the interface that the PDCP module uses to hand packets
 * to RX entities (both normal and bypass).
 */
class PdcpRxEntityBase : public omnetpp::cSimpleModule
{
  protected:
    void handleMessage(omnetpp::cMessage *msg) override;

  public:
    virtual void handlePacketFromLowerLayer(inet::Packet *pkt) = 0;
    virtual bool isEmpty() const { return true; }
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
};

} // namespace simu5g

#endif
