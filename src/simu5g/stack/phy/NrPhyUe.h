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

#include <map>

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/stack/phy/LtePhyUeD2D.h"

namespace simu5g {

class NrPhyUe : public LtePhyUeD2D
{

  protected:
    std::map<GHz, opp_component_ptr<LteChannelModel>> ntnChannelModel_;
    inet::ModuleRefByPar<LteChannelModel> primaryNtnChannelModel_;

    // reference to the parallel PHY layer
    inet::ModuleRefByPar<NrPhyUe> otherPhy_;

    void initialize(int stage) override;
    void initializeChannelModels();
    LteAirFrame *createAirFrame(const char *name, const UserControlInfo& lteInfo) override;
    void handleAirFrame(cMessage *msg) override;
    void sendUnicast(LteAirFrame *airFrame) override;
    void triggerHandover() override;
    void doHandover() override;

    // force handover to the given target node (0 means forcing detachment)
    virtual void forceHandover(MacNodeId targetMasterNode = NODEID_NONE, double targetMasterRssi = 0.0);
    void deleteOldBuffers(MacNodeId masterId);

  public:
    LteChannelModel *getReceptionChannelModel(const UserControlInfo *lteInfo) override;
};

} //namespace

#endif /* _NRPHYUE_H_ */
