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

#include <inet/common/ModuleRefByPar.h>
#include "simu5g/common/LteDefs.h"
#include "simu5g/common/LteCommon_m.h"

namespace simu5g {

using namespace omnetpp;

class Binder;
class LtePhyUe;
class LtePhyUeD2D;
class NrPhyUe;
class LteMacUe;
class LteAirFrame;
class UserControlInfo;
class LteRlcUm;
class LtePdcpBase;
class Ip2Nic;
class LteDlFeedbackGenerator;

class HandoverController : public cSimpleModule
{
private:
    LtePhyUe *phy_;
public:
    MacNodeId nodeId_ = NODEID_NONE;
    bool isNr_ = false;

    /** The current serving node */
    MacNodeId servingNodeId_ = NODEID_NONE;

    /** RSSI received from the current serving node */
    double servingNodeRssi_ = -999.0;

    /** ID of the not-master node from which the highest RSSI was received */
    MacNodeId candidateServingNodeId_;

    /** Highest RSSI received from not-master node */
    double candidateServingNodeRssi_ = -999.0;

    /**
     * Hysteresis threshold to evaluate handover: it introduces a small bias to
     * avoid multiple subsequent handovers.
     */
    double hysteresisThreshold_ = 0;

    /**
     * Value used to divide currentMasterRssi_ and create a hysteresisTh_.
     * Use zero to have hysteresisTh_ == 0.
     */
    double hysteresisFactor_;

    /**
     * Time interval elapsing from the reception of the first handover broadcast message
     * to the beginning of the handover procedure.
     * It must be a small number greater than 0 to ensure that all broadcast messages
     * are received before evaluating handover.
     * Note that broadcast messages for handover are always received at the very same time
     * (at beaconInterval_ seconds intervals).
     */
    // TODO: bring it to ned par!
    double handoverDelta_ = 0.00001;

    // Time for completion of the handover procedure
    double handoverLatency_;
    double handoverDetachmentTime_;
    double handoverAttachmentTime_;

    // Lower threshold of RSSI for detachment
    double minRssi_;

    bool hasCollector = false;

    /** Statistic for serving cell */
    static simsignal_t servingCellSignal_;

    /** Self message to trigger handover procedure evaluation */
    cMessage *handoverStarter_ = nullptr;

    /** Self message to start the handover procedure */
    cMessage *handoverTrigger_ = nullptr;

    /**
     * Handover switch
     */
    bool enableHandover_;

    inet::ModuleRefByPar<Binder> binder_;
    inet::ModuleRefByPar<LteMacUe> mac_;
    inet::ModuleRefByPar<LteRlcUm> rlcUm_;
    inet::ModuleRefByPar<LtePdcpBase> pdcp_;
    inet::ModuleRefByPar<Ip2Nic> ip2nic_;
    inet::ModuleRefByPar<LteDlFeedbackGenerator> fbGen_;
    inet::ModuleRefByPar<HandoverController> otherHandoverController_;

    ~HandoverController() override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void finish() override;
    void handleMessage(cMessage *msg) override;

    void setPhy(LtePhyUe *phy) {phy_ = phy;}
    LtePhyUe *getPhy() const {return phy_;}

    MacNodeId getNodeId() const { return nodeId_; }
    MacNodeId getServingNodeId() const { return servingNodeId_; }

    // called from handleAirFrame()
    void beaconReceived(LteAirFrame *frame, UserControlInfo *lteInfo);

    // invoked on self-message
    void triggerHandover();

    // invoked on self-message
    void doHandover();

    // helper
    void forceHandover(MacNodeId targetServingNodeId, double targetServingNodeRssi);

    // invoked from the above methods and from finish()
    void deleteOldBuffers(MacNodeId servingNodeId);

    // helper
    void updateHysteresisThreshold(double rssi);
};

} //namespace

#endif /* _LTE_HANDOVERCOORDINATOR_H_ */
