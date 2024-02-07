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

#ifndef _NRMACGNB_H_
#define _NRMACGNB_H_

#include "stack/mac/layer/LteMacEnbD2D.h"
#include "stack/mac/layer/LteMacEnb.h"

class NRMacGnb : public LteMacEnbD2D
{
  protected:


  public:

    NRMacGnb();
    virtual ~NRMacGnb();

    /**
     * Reads MAC parameters and performs initialization.
     */
    virtual void initialize(int stage);

    virtual void handleMessage(inet::cMessage* msg);

    //virtual bool bufferizePacket(inet::cPacket* pkt);

    //virtual void handleUpperMessage(cPacket* pkt);
};

#endif
