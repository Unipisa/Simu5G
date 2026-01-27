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

#ifndef _LTE_HANDOVERCONTROLLER_H_
#define _LTE_HANDOVERCONTROLLER_H_

#include "simu5g/common/LteDefs.h"
#include "simu5g/common/LteCommon_m.h"

namespace simu5g {

using namespace omnetpp;

class LtePhyUe;
class LtePhyUeD2D;
class NrPhyUe;
class LteAirFrame;
class UserControlInfo;

class HandoverController : public cSimpleModule
{
private:
    LtePhyUe *phy_;
public:
    ~HandoverController() override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void handleMessage(cMessage *msg) override;

    void setPhy(LtePhyUe *phy) {phy_ = phy;}

    // called from handleAirFrame()
    void LtePhyUe_handoverHandler(LteAirFrame *frame, UserControlInfo *lteInfo);

    // invoked on self-message
    void LtePhyUe_triggerHandover();
    void LtePhyUeD2D_triggerHandover();
    void NrPhyUe_triggerHandover();

    // invoked on self-message
    void LtePhyUe_doHandover();
    void LtePhyUeD2D_doHandover();
    void NrPhyUe_doHandover();

    // helper
    void NrPhyUe_forceHandover(MacNodeId targetMasterNode, double targetMasterRssi);

    // invoked from the above methods and from finish()
    void LtePhyUe_deleteOldBuffers(MacNodeId masterId);
    void NrPhyUe_deleteOldBuffers(MacNodeId masterId);

    // helper
    double updateHysteresisTh(double v);
};

} //namespace

#endif /* _LTE_HANDOVERCOORDINATOR_H_ */
