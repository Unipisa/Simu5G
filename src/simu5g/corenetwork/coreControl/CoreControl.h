//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _CORECONTROL_H_
#define _CORECONTROL_H_

#include <map>
#include <string>
#include <vector>

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/binder/Binder.h"
#include "simu5g/corenetwork/gtp/GtpTunnel.h"

namespace simu5g {

class BearerConfigurator;
class ConnectionControlEnb;
class UserPlaneNodeControl;

/**
 * The control plane of the core network, one per cellular network: the MME and the
 * gateways' control plane of an EPC, or the AMF and the SMF of a 5G core, in one
 * module.
 * It keeps the UEs' sessions and programs the nodes that end their tunnels
 * through the nodes' control-plane entry points (UserPlaneNodeControl, and the base
 * stations' ConnectionControlEnb). See CoreControl.ned.
 */
class CoreControl : public omnetpp::cSimpleModule, public omnetpp::cListener
{
  protected:
    inet::ModuleRefByPar<Binder> binder_;
    inet::ModuleRefByPar<BearerConfigurator> bearerConfigurator_;

    // A node of the network that ends tunnels (see takeGtpEndpoints()), through its
    // control-plane entry point: the UserPlaneNodeControl of a user plane node (a UPF/PGW or a
    // MEC host's UPF), or the connection control of a base station. Each allocates
    // its node's TEIDs itself, and is programmed through its calls.
    struct GtpEndpoint {
        omnetpp::cModule *node = nullptr;     // the network node
        UserPlaneNodeControl *userPlaneNode = nullptr;  // user plane nodes only
        ConnectionControlEnb *bs = nullptr;   // base stations only
        CoreNodeType type = ENB;
        MacNodeId bsId = NODEID_NONE;         // base stations only
        std::string gateway;                  // the core network gateway of a base station connected to the core network, or of a MEC host's UPF; empty otherwise
    };
    std::vector<GtpEndpoint> gtpEndpoints_;       // the user plane nodes, then the base stations
    std::map<MacNodeId, int> bsGtpEndpoints_;     // base station id -> index into gtpEndpoints_

    // A UE's session (a PDN connection of an EPC, a PDU session of a 5G core, TS 23.501
    // 5.6), as the core network keeps it: one per UE, of the type the UE requested,
    // established when the UE first has a serving node, released when the UE leaves
    // (see establishSession()). The
    // anchor (the PGW, or the PDU session anchor UPF) is chosen at establishment and
    // kept for the lifetime of the session (SSC mode 1): a handover only moves the
    // downlink end of the tunnel (see updateDownlinkPath()). The tunnel ends are told
    // about every change through their nodes' control-plane entry points.
    struct CoreSession {
        omnetpp::cModule *ueModule = nullptr;
        SessionRef ref;                          // the UE's node ids and the session id
        SessionType type = IP_V4;                   // the session's type, as the UE requested it
        int anchor = -1;                            // the anchor UPF/PGW, index into gtpEndpoints_
        FTeid ulAnchor;                             // uplink F-TEID at the anchor
        std::map<int, FTeid> ulMecHosts;            // uplink F-TEIDs at the MEC host UPFs of the anchor's core network, by index into gtpEndpoints_
        MacNodeId dlBaseStation = NODEID_NONE;      // where the downlink enters the RAN; NODEID_NONE while the UE is attached nowhere
        MacNodeId lastDlBaseStation = NODEID_NONE;  // where the downlink last entered the RAN, kept while the UE is attached nowhere
        FTeid dl;                                   // downlink F-TEID at dlBaseStation
        std::map<MacNodeId, FTeid> dlTunnels;       // the downlink F-TEID at each base station the UE has been attached through, kept until release
    };
    typedef std::pair<int, SessionId> CoreSessionKey;     // the UE module's id, and the session id
    std::map<CoreSessionKey, CoreSession> sessions_;
    std::map<MacNodeId, CoreSessionKey> sessionOfNode_;   // UE node id (either stack) -> the UE's session

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    void handleMessage(omnetpp::cMessage *msg) override { throw omnetpp::cRuntimeError("This module does not process messages"); }

    /**
     * Binder::nodeUnregisteredSignal_: a departing UE's session is released.
     */
    void receiveSignal(omnetpp::cComponent *source, omnetpp::simsignal_t signalID, long nodeId, omnetpp::cObject *details) override;

    // Take the nodes that end tunnels from the Binder: the user plane nodes' control-
    // plane entry points, registered there at INITSTAGE_LOCAL, in registration order, then the
    // base stations' connection controls, through the node directory, in node id
    // order. Before the first tunnel is set up.
    virtual void takeGtpEndpoints();

    // Deliver the QFI classification rules the BearerConfigurator holds to their
    // evaluation sites: the downlink rules to each user plane node, the uplink rules
    // to each SDAP UE's classifier through the
    // UE's RRC (the QoS rules NAS signaling installs into a UE at PDU session
    // establishment, TS 23.501 5.7.1.4). The sites never author rules of their own.
    virtual void deliverQfiRules();

    // The node a gateway parameter names, or nullptr
    virtual omnetpp::cModule *findGatewayNode(const std::string& gateway);

    // The UPF/PGW endpoint the gateway parameter of the given endpoint names; throws if
    // there is none
    virtual int findGatewayEndpoint(const std::string& gateway, const GtpEndpoint& from);

    // The base station a downlink packet for the UE enters the RAN at: the master of the
    // serving node of the stack the core network addresses the UE by, the LTE one while
    // it is attached and else the NR one (as Binder::getMacNodeId() resolves a UE
    // address); NODEID_NONE if the UE is attached nowhere
    virtual MacNodeId findDlBaseStation(MacNodeId lteNodeId, MacNodeId nrNodeId);

    // Establish the session of the UE with the given node id, of the type the UE
    // requested, anchored at the gateway of its downlink base station: its uplink
    // tunnels at the anchor and the MEC host UPFs. Does nothing if the UE is attached
    // nowhere yet, or its base station is not connected to a core network; throws if
    // the anchor's core network cannot carry a session of the type (an EPC carries the
    // IP types only). The RAN end of the session is set up separately
    // (initialUeMessage(), pathSwitchRequest()).
    virtual void establishSession(MacNodeId ueNodeId, SessionType type);

    // If the base station the UE's downlink enters the RAN at has changed, the downlink
    // end of the tunnel moves there (the path switch, TS 23.502 4.9.1.2.2), and that
    // base station is told (ConnectionControlEnb::downlinkPathSwitched())
    virtual void updateDownlinkPath(CoreSession& session);

    // Set up the session's tunnels at a base station the UE is attached through, unless
    // they are there already (see registerRanTunnel())
    virtual void setUpRanTunnels(CoreSession& session, MacNodeId bsId);

    // Record the session's downlink tunnel at a base station. The base stations of the
    // session learn each other's downlink TEIDs, to forward the downlink over X2 with.
    virtual void registerRanTunnel(CoreSession& session, MacNodeId bsId, const FTeid& dl);

    // The session's uplink tunnels, as a base station uses them
    virtual UplinkTunnels getUplinkTunnels(const CoreSession& session);

    // Release the session of the UE with the given node id, if it has one
    virtual void releaseSession(MacNodeId ueNodeId);

  public:
    // INITIAL UE MESSAGE (S1AP/NGAP), and the registration and session establishment that
    // follow: a leg of a UE has connected at the given base station, requesting a
    // session of the given type (the NAS message the INITIAL UE MESSAGE carries). The
    // UE's session is established once, at its first leg's registration; the other
    // leg of a dual-stack UE requests the same type. The session's RAN resources are
    // set up at every base station the UE attaches through, once
    // (ConnectionControlEnb::sessionResourceSetup()); and the downlink path is
    // settled.
    virtual void initialUeMessage(MacNodeId legId, ConnectionControlEnb *bs, SessionType sessionType);

    // PATH SWITCH REQUEST (S1AP/NGAP), from the base station a leg arrived at by handover: the
    // UE's session now enters the RAN at that base station (the master of it, under
    // dual connectivity), with the downlink F-TEIDs the handover preparation set up
    // there (the session resources "to be switched in downlink"); a base station the
    // preparation gave no tunnels (a secondary node's source has none to transfer)
    // gets them now. The downlink path is switched if the base station the downlink
    // enters at changed.
    virtual void pathSwitchRequest(MacNodeId legId, ConnectionControlEnb *bs, const std::vector<SessionResource>& sessions);

    // UE CONTEXT RELEASE REQUEST (S1AP/NGAP), from the base station a leg left without a
    // handover: the leg is attached nowhere; the downlink path follows the UE's
    // remaining attachment, if any. The UE's session stays until the UE leaves the
    // simulation (nodeUnregistered).
    virtual void ueContextReleaseRequest(MacNodeId legId);
};

} //namespace

#endif
