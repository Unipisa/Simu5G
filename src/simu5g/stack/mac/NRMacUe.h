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

#ifndef _NRMACUED2D_H_
#define _NRMACUED2D_H_

#include "stack/mac/LteMacUeD2D.h"

namespace simu5g {

class NRMacUe : public LteMacUeD2D
{

  protected:

    /**
     * Main loop
     */
    void handleSelfMessage() override;

    /**
     * macSduRequest() sends a message to the RLC layer
     * requesting MAC SDUs (one for each CID),
     * according to the Schedule List.
     */
    int macSduRequest() override;

    /**
     * macPduMake() creates MAC PDUs (one for each CID)
     * by extracting SDUs from Real Mac Buffers according
     * to the Schedule List.
     * It sends them to H-ARQ (at the moment lower layer)
     *
     * On UE it also adds a BSR control element to the MAC PDU
     * containing the size of its buffer (for that CID)
     */
    void macPduMake(MacCid cid = 0) override;
};

} //namespace

#endif

