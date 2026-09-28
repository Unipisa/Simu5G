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
#include "simu5g/corenetwork/coreControl/CoreControl.h"
#include "simu5g/stack/rrc/ConnectionControlBase.h"

namespace simu5g {

using namespace omnetpp;

class Binder;
class PhyUe;
class LteMacUe;
class LteAmc;
class LteAirFrame;
class UserControlInfo;
class BearerManagement;
class ConnectionControlEnb;
class HandoverPacketHolderUe;
class LteDlFeedbackGenerator;

class HandoverController : public ConnectionControlBase
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
    inet::ModuleRefByPar<CoreControl> coreControl_;

  protected:
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void initialize(int stage) override;
    void finish() override;
    void handleMessage(cMessage *msg) override;

    // The handoverStarter timer fired: the DC leg coordination, then a handover is
    // reported to the serving base station (measurementReport), which commands it
    // (handoverCommand()); an attachment from nowhere or a detachment starts here
    virtual void triggerHandover();
    // Start the handover, attachment or detachment: the ledger moves ahead, the uplink
    // is held, the handoverTrigger timer runs the execution (doHandover())
    virtual void startHandover();
    virtual void doHandover();
    /**
     * Tear down this UE's own side of the bearer state it shares with @p servingNodeId:
     * its MAC queues and RLC/PDCP entities toward that node (the node releases its own
     * side when told, see ConnectionControlEnb::ueContextRelease()/connectionLost()).
     *
     * @param localNodeIsBeingDeleted  true when the whole UE module tree is being deleted
     *        mid-simulation (see finish()). The local RLC/PDCP entity modules are then left
     *        alone: they are submodules of the NIC that is about to be destroyed anyway, and
     *        deleting them here would mutate the submodule list that OMNeT++'s callFinish()
     *        is enumerating, which aborts the run with "SubmoduleIterator: Submodule
     *        insertion/deletion detected".
     */
    virtual void deleteOwnBuffers(MacNodeId servingNodeId, bool localNodeIsBeingDeleted = false);
    virtual void updateHysteresisThreshold(double rssi);
    virtual LteAmc *getAmcModule(MacNodeId nodeId);

    // The control-plane entry point of a base station, through the Binder's node
    // directory; throws if there is none (the UE is attached nowhere)
    virtual ConnectionControlEnb *baseStationControl(MacNodeId bsId);
    // The base station that establishes a flow's bearers: the one the flow names as
    // its infrastructure end, else (a sidelink flow) the serving node of the source
    virtual ConnectionControlEnb *baseStationFor(const FlowId& flow);

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

    // RRCReconfiguration with sync, from the serving base station: this leg hands over
    // to the given cell (the base station's answer to the leg's measurementReport)
    virtual void handoverCommand(MacNodeId targetNodeId);

    // The UE's control-plane entry point (ConnectionControlBase): the node's data
    // path asks for bearers here, and this leg's serving base station -- which is
    // where they are established -- is asked in turn
    DrbId establishBearer(const FlowId& flow, const FlowBindingKey& key, const inet::Packet *pkt) override;
    DrbId establishBearer(const FlowId& flow, const BearerRequest& req) override;
    DrbId resolveDrbForQfi(MacNodeId ueNodeId, Qfi qfi) override;
    // An infrastructure bearer's identity is its base station's to release; nothing
    // to do here (the D2D subclass returns a sidelink bearer's to the sidelink pool)
    void bearerReleased(DrbKey bearer) override;
    void multicastGroupJoined(MacNodeId nodeId, MacNodeId groupId) override;
    // Bearer installation, from the serving base station: each forwards to the UE's
    // BearerManagement
    void configureDrb(const DrbDesc& drb) override;
    void createIncomingConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp) override;
    void createOutgoingConnection(const FlowId& flow, const BearerRequest& req, bool withPdcp) override;
    void setUplinkQfiRules(QfiRuleSet&& rules) override;
};

} //namespace

#endif /* _LTE_HANDOVERCONTROLLER_H_ */
