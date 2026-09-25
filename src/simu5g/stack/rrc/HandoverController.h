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
#include "simu5g/common/LteTypes.h"
#include "simu5g/corenetwork/bearerConfigurator/BearerConfigurator.h"

namespace simu5g {

using namespace omnetpp;

class Binder;
class PhyUe;
class LteMacUe;
class LteAmc;
class LteAirFrame;
class UserControlInfo;
class BearerManagement;
class HandoverPacketHolderUe;
class LteDlFeedbackGenerator;

class HandoverController : public cSimpleModule
{
  protected:
    PhyUe *phy_;

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
    BearerManagement *bearerManagement_ = nullptr;
    inet::ModuleRefByPar<HandoverPacketHolderUe> handoverPacketHolder_;
    inet::ModuleRefByPar<LteDlFeedbackGenerator> fbGen_;
    inet::ModuleRefByPar<HandoverController> otherHandoverController_;
    inet::ModuleRefByPar<BearerConfigurator> bearerConfigurator_;

  protected:
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void finish() override;
    void handleMessage(cMessage *msg) override;

    virtual void triggerHandover();
    virtual void doHandover();
    /**
     * Tear down the bearer state this UE shares with @p servingNodeId, at both ends.
     *
     * @param localNodeIsBeingDeleted  true when the whole UE module tree is being deleted
     *        mid-simulation (see finish()). The local RLC/PDCP entity modules are then left
     *        alone: they are submodules of the NIC that is about to be destroyed anyway, and
     *        deleting them here would mutate the submodule list that OMNeT++'s callFinish()
     *        is enumerating, which aborts the run with "SubmoduleIterator: Submodule
     *        insertion/deletion detected". The peer-side and global state still has to go.
     */
    virtual void deleteOldBuffers(MacNodeId servingNodeId, bool localNodeIsBeingDeleted = false);
    virtual void updateHysteresisThreshold(double rssi);
    virtual LteAmc *getAmcModule(MacNodeId nodeId);

    /// True if this UE is a dual-stack one, i.e. it has a second stack ("leg") whose
    /// handover controller this one must coordinate with in DC scenarios.
    bool hasOtherLeg() const { return otherHandoverController_ != nullptr; }

    /// Handover lifecycle notification hooks. The base implementations are empty;
    /// HandoverControllerD2D overrides them with the D2D-specific behavior.
    /// Called by triggerHandover() once the handover decision is made, before the handover latency starts.
    virtual void onHandoverStarting();
    /// Called by doHandover() before buffers are deleted and the user is re-attached to the new cell's AMC.
    virtual void onHandoverExecuting();
    /// Called when the (delayed) handover-completion notification fires.
    virtual void onHandoverCompleted();
    /// Called by finish() when the UE's module tree is being deleted mid-simulation, after
    /// the serving cell's AMC has been detached from the UL and DL directions. It is the
    /// departure counterpart of onHandoverExecuting(): whatever that hook detaches on leaving
    /// the old cell has to be detached here too, since the UE is leaving for good.
    virtual void onNodeLeaving();

  public:
    ~HandoverController() override;

    void setPhy(PhyUe *phy) {phy_ = phy;}
    PhyUe *getPhy() const {return phy_;}

    MacNodeId getNodeId() const { return nodeId_; }
    MacNodeId getServingNodeId() const { return servingNodeId_; }

    /**
     * Called from PHY on reception of a beacon signal
     */
    virtual void beaconReceived(LteAirFrame *frame, UserControlInfo *lteInfo);

    /**
     * Used in a DC setup. Called by a HandoverController to force the
     * other one to do the handover.
     */
    virtual void forceHandover();
};

} //namespace

#endif /* _LTE_HANDOVERCONTROLLER_H_ */
