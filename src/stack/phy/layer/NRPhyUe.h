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

#ifndef _NRPHYUE_H_
#define _NRPHYUE_H_

#include "stack/phy/layer/LtePhyUeD2D.h"

class NRPhyUe : public LtePhyUeD2D
{

  protected:

    // reference to the parallel PHY layer
    NRPhyUe* otherPhy_;

    virtual void initialize(int stage);
    virtual void handleAirFrame(cMessage* msg);
    virtual void triggerHandover();
    virtual void doHandover();

    // force handover to the given target node (0 means forcing detachment)
    virtual void forceHandover(MacNodeId targetMasterNode=0, double targetMasterRssi=0.0);
    void deleteOldBuffers(MacNodeId masterId);

  public:
    NRPhyUe();
    virtual ~NRPhyUe();
};

#endif  /* _NRPHYUE_H_ */
