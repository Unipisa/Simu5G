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

#ifndef _NRTXPDCPENTITY_H_
#define _NRTXPDCPENTITY_H_

#include "simu5g/stack/pdcp/LteTxPdcpEntity.h"
#include "simu5g/stack/dualConnectivityManager/DualConnectivityManager.h"

namespace simu5g {

class LtePdcpBase;

/**
 * @class NrTxPdcpEntity
 * @brief Entity for New Radio PDCP Layer
 *
 * This is the PDCP entity of LTE/NR Stack.
 *
 * PDCP entity performs the following tasks:
 * - maintain numbering of one logical connection
 *
 */
class NrTxPdcpEntity : public LteTxPdcpEntity
{
  protected:

    // deliver the PDCP PDU to the lower layer or to the X2
    void deliverPdcpPdu(Packet *pkt) override;

  public:


    void initialize() override;
};

} //namespace

#endif

