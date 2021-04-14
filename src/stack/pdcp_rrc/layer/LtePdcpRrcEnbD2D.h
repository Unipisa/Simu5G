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

#ifndef _LTE_LTEPDCPRRCENBD2D_H_
#define _LTE_LTEPDCPRRCENBD2D_H_

#include <omnetpp.h>
#include "stack/pdcp_rrc/layer/LtePdcpRrc.h"

/**
 * @class LtePdcp
 * @brief PDCP Layer
 *
 * This is the PDCP/RRC layer of LTE Stack (with D2D support).
 *
 */
class LtePdcpRrcEnbD2D : public LtePdcpRrcEnb
{

  protected:

    virtual void initialize(int stage) override;
    virtual void handleMessage(omnetpp::cMessage* msg) override;

    /**
     * handler for data port
     * @param pkt incoming packet
     */
    virtual void fromDataPort(omnetpp::cPacket *pkt) override;

    void pdcpHandleD2DModeSwitch(MacNodeId peerId, LteD2DMode newMode);

  public:

};

#endif
