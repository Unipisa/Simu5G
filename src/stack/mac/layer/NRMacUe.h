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

#include "stack/mac/layer/LteMacUeD2D.h"

class NRMacUe : public LteMacUeD2D
{


  protected:

    /**
     * Main loop
     */
    virtual void handleSelfMessage();

    /**
     * macPduMake() creates MAC PDUs (one for each CID)
     * by extracting SDUs from Real Mac Buffers according
     * to the Schedule List.
     * It sends them to H-ARQ (at the moment lower layer)
     *
     * On UE it also adds a BSR control element to the MAC PDU
     * containing the size of its buffer (for that CID)
     */
    virtual void macPduMake(MacCid cid=0);

  public:
    NRMacUe();
    virtual ~NRMacUe();
};

#endif
