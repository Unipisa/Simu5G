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

#include <inet/common/ModuleRefByPar.h>

#include "stack/phy/LtePhyUeD2D.h"

namespace simu5g {

class NRPhyUe : public LtePhyUeD2D
{

  protected:

    // reference to the parallel PHY layer
    inet::ModuleRefByPar<NRPhyUe> otherPhy_;

    void initialize(int stage) override;
    void handleAirFrame(cMessage *msg) override;
    void triggerHandover() override;
    void doHandover() override;

    // force handover to the given target node (0 means forcing detachment)
    virtual void forceHandover(MacNodeId targetMasterNode = NODEID_NONE, double targetMasterRssi = 0.0);
    void deleteOldBuffers(MacNodeId masterId);
};

} //namespace

#endif /* _NRPHYUE_H_ */

