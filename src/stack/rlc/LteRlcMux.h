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

#ifndef _LTE_LTERLCMUX_H_
#define _LTE_LTERLCMUX_H_

#include <omnetpp.h>
#include "common/LteCommon.h"
#include "common/LteControlInfo.h"

/**
 * \class LteRLC
 * \brief RLC Layer
 *
 * This is the Muxer of RLC layer of LTE Stack.
 * It has the function of muxing/demuxing traffic:
 * - Traffic coming from TM/UM/AM modules is decoded
 *   and forwarded to MAC layer ports
 * - Traffic coming from MAC layer is decoded
 *   and forwarded to TM/UM/AM modules
 *
 */
class LteRlcMux : public omnetpp::cSimpleModule
{
  public:
    virtual ~LteRlcMux()
    {
    }

  protected:

    /**
     * Initialize class structures
     * gatemap and delay
     */
    virtual void initialize() override;

    /**
     * Analyze gate of incoming packet
     * and call proper handler
     */
    virtual void handleMessage(omnetpp::cMessage *msg) override;

    /**
     * Statistics recording
     */
    virtual void finish() override;

  private:
    /*
     * Upper Layer Handler
     */

    /**
     * handler for rlc2mac packets
     *
     * @param pkt packet to process
     */
    void rlc2mac(omnetpp::cPacket *pkt);

    /*
     * Lower Layer Handler
     */

    /**
     * handler for mac2rlc packets
     *
     * @param pkt packet to process
     */
    void mac2rlc(omnetpp::cPacket *pkt);

    /*
     * Data structures
     */

    omnetpp::cGate* macSap_[2];
    omnetpp::cGate* tmSap_[2];
    omnetpp::cGate* umSap_[2];
    omnetpp::cGate* amSap_[2];
};

#endif
