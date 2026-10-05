//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef STACK_PHY_CHANNELMODEL_STOCHASTICCHANNELMODEL_H_
#define STACK_PHY_CHANNELMODEL_STOCHASTICCHANNELMODEL_H_

#include "simu5g/common/LteDefs.h"
#include "simu5g/stack/phy/channelmodel/ChannelModelBase.h"

namespace simu5g {

using namespace omnetpp;

class Binder;
class CellularTransmission;
class PathLossModel;

/**
 * The full PHY link model: everything between a transmitted air frame and the
 * decision on whether it was received.
 *
 * The impairments are modeled statistically -- drawn from the distributions of a
 * 3GPP propagation study, rather than computed from the geometry of an actual
 * environment -- which is what the name refers to, and what sets this model apart
 * from IdealChannelModel, its impairment-free sibling. It is far more than
 * propagation, and covers:
 * - path loss per deployment scenario, LOS/NLOS state, and log-normal shadowing;
 * - multipath fading, Jakes or Rayleigh;
 * - the antenna pattern attenuation and the link budget (the two radios'
 *   antenna gains, the receiver's cable loss and noise figure; thermal noise);
 * - interference from other cells -- downlink, uplink, external cells and
 *   background cells;
 * - the assembly of all of the above into a per-band SINR, and the mapping of
 *   that SINR onto a block error probability (with HARQ reduction) that decides
 *   reception;
 * - the mobility the correlated quantities need: the position history and
 *   speed of a node, and where a link's ends were when its LOS state and its
 *   shadowing were last drawn;
 * - the SINR statistics.
 *
 * The radio medium keeps the state the evaluations carry from one to the next:
 * whether each link is in line of sight, its shadowing and its Jakes fading
 * paths, once per link and shared by every channel model evaluating it
 * (losMap(), shadowingMap(), jakesFadingMap()), and this model's own position
 * histories (channelState()).
 *
 * The propagation formulas proper live in a PathLossModel strategy (pathLoss_)
 * that this class owns and delegates to from computePathLoss, computeLosProbability,
 * computeShadowing and computeAngularAttenuation. Which 3GPP propagation study
 * the strategy implements is chosen by the pathLossType parameter ("Tr36814",
 * "Tr36873" or "Tr38901"); createPathLossModel() instantiates the matching
 * strategy class. Tr36873ChannelModel and Tr38901ChannelModel are NED-level
 * presets of this class (no C++ class of their own) that only override the
 * pathLossType default, to Tr36873 and Tr38901 respectively. All the rest --
 * fading, interference, SINR assembly, the reception decision -- is shared
 * by every pathLossType.
 *
 * Supported propagation studies:
 * - 3GPP TR 36.814, "Further advancements for E-UTRA physical layer aspects", v9.2.0, March 2017
 * - 3GPP TR 36.873, "Study on 3D channel model for LTE", v12.7.0, December 2017
 * - 3GPP TR 38.901, "Study on channel model for frequencies from 0.5 to 100 GHz", v16.1.0, December 2019
 * - 3GPP TS 36.211, "LTE; Physical channels and modulation", v13.2.0, June 2016
 *
 * covering the Indoor Hotspot (InH), Urban Microcell (UMi), Urban Macrocell
 * (UMa), Rural Macrocell (RMa) and Suburban Macrocell (SMa) deployment
 * scenarios (not every study covers every scenario; see the PathLossModel
 * subclasses).
 *
 * D2D links are not evaluated here. The D2dChannelModel subclass layers them on
 * top of this class.
 */
class StochasticChannelModel : public ChannelModelBase
{
  protected:

    // eNodeB Height
    double hNodeB_;

    // UE Height
    double hUe_;

    // average Building Heights
    double hBuilding_;

    // flag for using high-loss or low-loss model for building penetration
    // see table 7.4.3-2 in TR 38.901
    bool useBuildingPenetrationHighLossModel_;

    // Average street's width
    double wStreet_;

    // enable/disable the shadowing
    bool shadowing_;

    // enable/disable intercell interference computation
    bool enableBackgroundCellInterference_;
    bool enableExtCellInterference_;
    bool enableDownlinkInterference_;
    bool enableUplinkInterference_;

    bool enable_extCell_los_;

    typedef ChannelState::Position Position;
    typedef ChannelState::JakesFadingData JakesFadingData;
    typedef ChannelState::JakesFadingVector JakesFadingVector;
    typedef ChannelState::JakesFadingMap JakesFadingMap;
    typedef ChannelState::LosMap LosMap;
    typedef ChannelState::ShadowingSample ShadowingSample;
    typedef ChannelState::ShadowFadingMap ShadowFadingMap;

    ChannelState *channelState_ = nullptr; // this model's channel state, kept by the radio medium; see channelState()
    LosMap *losMap_ = nullptr; // the LOS state of the links on this model's carrier, kept by the radio medium; see losMap()
    ShadowFadingMap *shadowingMap_ = nullptr; // the shadowing of the links on this model's carrier, kept by the radio medium; see shadowingMap()
    JakesFadingMap *jakesFadingMap_ = nullptr; // the Jakes fading paths of the links on this model's carrier, kept by the radio medium; see jakesFadingMap()

    // Scenario
    DeploymentScenario scenario_;

    // Formulas of the selected 3GPP propagation study; owned, created in initialize()
    PathLossModel *pathLoss_ = nullptr;

    // Correlation distance used in shadowing computation and
    // also used to recompute the probability of LOS
    double correlationDistance_;

    // Antenna gain of eNodeB
    double antennaGainEnB_;

    // Antenna gain of UE
    double antennaGainUe_;

    // Thermal noise
    double thermalNoise_;

    // Cable loss
    double cableLoss_;

    // UE noise figure
    double ueNoiseFigure_;

    // eNodeB noise figure
    double bsNoiseFigure_;

    // Enable disable fading
    bool fading_;

    // Number of fading paths in Jakes fading
    int fadingPaths_;

    // Average delay spread in Jakes fading
    double delayRMS_;

    bool tolerateMaxDistViolation_;

    enum FadingType
    {
        RAYLEIGH, JAKES
    };

    // Fading type (JAKES or RAYLEIGH)
    FadingType fadingType_;

    // Enable or disable the dynamic computation of LOS NLOS probability for each user
    bool dynamicLos_;

    // If dynamicLos is false this boolean is initialized to true if all users will be in LOS or false otherwise
    bool fixedLos_;

    // If false, disable the collection of SINR statistics, which might be quite time-consuming
    bool collectSinrStatistics_;

  public:
    ~StochasticChannelModel() override;

    void initialize(int stage) override;

    /*
     * Compute attenuation (path loss + optional shadowing) over a radio link.
     */
    double getAttenuation(const RadioLink& link) override;


    /*
     * Convenience overload for the cellular callers that still think in
     * (UE, direction, remote coordinate) terms -- the interference helpers and
     * the background-UE path. Builds a cellular link and forwards, so the
     * subclass override of getAttenuation(const RadioLink&) still applies.
     *
     * @param nodeId mac node id of UE
     * @param dir traffic direction
     * @param coord position of end point communication (if dir==UL is the position of UE else is the position of eNodeB)
     */
    double getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord coord)
    {
        return getAttenuation(cellularLink(nodeId, dir, coord));
    }

    /*
     *  Compute angle between two coordinates
     *
     * @param center first coord
     * @param point second coord
     */
    virtual double computeAngle(Coord center, Coord point);

    /*
     *  Compute vertical angle between two coordinates
     *  The returned angle is the angle with respect to the zenith direction
     *
     * @param center first coord
     * @param point second coord
     * @return angle
     */
    virtual double computeVerticalAngle(Coord center, Coord point);

    /*
     *  Compute Attenuation caused by transmission direction
     *
     * @param angle angle
     */
    virtual double computeAngularAttenuation(double hAngle, double vAngle = 0);

    /*
     * Compute the shadowing of a link: its first sample is drawn, and it is
     * redrawn, correlated with the previous sample, when either of its radios
     * has moved more than the correlation distance since that sample was drawn
     *
     * @param d3D 3D distance between UE and eNodeB
     * @param d2D 2D distance between UE and eNodeB
     * @param los whether the link is in line of sight, which selects the standard deviation
     * @param link the link, whose linkKey keys the sample and whose ends' positions it is drawn at
     */
    virtual double computeShadowing(double d3D, double d2D, bool los, const RadioLink& link);

    /*
     * Compute sinr for each band for user nodeId according to pathloss, shadowing (optional) and multipath fading
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    std::vector<double> getSINR(AirFrame *frame, UserControlInfo *lteInfo) override;

    /*
     * Add noise and interference to an already-computed per-band received-power
     * vector. Split out so that a caller which already holds the RSRP (the D2D
     * one-to-many capture-effect path) shares this implementation instead of
     * repeating it.
     */
    virtual std::vector<double> getSINR(const RadioLink& link, UserControlInfo *lteInfo, std::vector<double> snrVector);

    /*
     * Compute received useful signal for each band for user nodeId according to pathloss, shadowing (optional) and multipath fading
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    std::vector<double> getRSRP(AirFrame *frame, UserControlInfo *lteInfo) override;

    /*
     * Compute sinr for each band for a background UE according to pathloss
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    std::vector<double> getSINR_bgUe(AirFrame *frame, UserControlInfo *lteInfo, double speed) override;

    /*
     * Compute the error probability of the transmitted packet according to cqi used, txmode, and the received power
     * after that it throws a random number in order to check if this packet will be corrupted or not
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     * @param rsrpVector the received signal for each RB, if it has already been computed
     */
    bool isReceptionSuccessful(AirFrame *frame, UserControlInfo *lteI, const std::vector<double>& rsrpVector) override;

    /*
     * Compute the path-loss attenuation according to the selected scenario
     *
     * @param distance between UE and eNodeB
     * @param los line-of-sight flag
     */
    double computePathLoss(double distance, double dbp, bool los) override;

    /*
     * Compute Rayleigh fading
     *
     * @param i index in the trace file
     * @param nodeid mac node id of UE
     */
    virtual double rayleighFading(MacNodeId id, unsigned int band);

    /*
     * Compute the Jakes fading of one band of a link. The link's fading paths
     * are drawn into jakesMap, every band's at once, when it is first
     * evaluated; the channel model's evaluations use jakesFadingMap(), kept
     * once per link.
     *
     * @param jakesMap the fading paths, by link
     * @param key the link
     * @param speed speed of UE
     * @param band logical band id
     */
    virtual double jakesFading(JakesFadingMap& jakesMap, const LinkKey& key, double speed, unsigned int band);

    /*
     * Decide whether the link is in line of sight -- drawn against the LOS
     * probability, or fixedLos if dynamicLos is off -- and record it in losMap(),
     * with where the link's ends are
     *
     * @param d3D 3D distance between UE and eNodeB
     * @param d2D 2D distance between UE and eNodeB
     * @param link the link
     */
    virtual void computeLosProbability(double d3D, double d2D, const RadioLink& link);

    bool isUplinkInterferenceEnabled() override { return enableUplinkInterference_; }
    /*
     * Compute the received useful signal (RSRP) per band over a radio link.
     */
    virtual std::vector<double> getRSRP(const RadioLink& link, double txPower);

  protected:

    /*
     * Create the strategy object supplying the propagation formulas
     * (pathLoss_), chosen by the pathLossType parameter.
     */
    virtual PathLossModel *createPathLossModel();

    /*
     * This model's channel state, which the radio medium keeps. Resolved on
     * first use, since the medium is not known before INITSTAGE_SIMU5G_POSTLOCAL.
     */
    ChannelState& channelState();
    const ChannelState& channelState() const;

    /*
     * Whether each link on this model's carrier is in line of sight, by link
     * (RadioLink::linkKey), which the radio medium keeps once per link for
     * every channel model evaluating it. Resolved on first use.
     */
    LosMap& losMap();

    /*
     * The last shadowing sample of each link on this model's carrier, by link
     * (RadioLink::linkKey), which the radio medium keeps once per link for
     * every channel model evaluating it. Resolved on first use.
     */
    ShadowFadingMap& shadowingMap();

    /*
     * The Jakes fading paths of each link on this model's carrier, by link
     * (RadioLink::linkKey), which the radio medium keeps once per link for
     * every channel model evaluating it. Resolved on first use.
     */
    JakesFadingMap& jakesFadingMap();

    /*
     * Build the RadioLink described by a frame's control info (DL, UL, and the
     * feedback variants), the local radio at the given position (see
     * receptionPosition()). The D2D path builds its links separately -- its API
     * takes the peer endpoint explicitly rather than deriving it.
     */
    virtual RadioLink linkFor(UserControlInfo *lteInfo, const inet::Coord& localPosition);

    /*
     * Where the local radio was as the frame started to arrive, which is where
     * its reception is evaluated; the radio's current position for a frame
     * that did not arrive through it (a frame built to ask what a reception
     * would be).
     */
    const inet::Coord& receptionPosition(const AirFrame *frame) const;

    /*
     * Build the RadioLink for a UE<->serving-BS link expressed the old way: the
     * local module is one endpoint, 'coord' the other, and 'dir' says which of
     * the two is the UE.
     */
    RadioLink cellularLink(MacNodeId ueId, Direction dir, inet::Coord coord);

    /*
     * Emit the received-SINR statistic for a decoded frame, on the UE's
     * receiver: this node's for a downlink reception, the sending UE's for any
     * other. The D2D model overrides it to route D2D/D2D_MULTI receptions to
     * rcvdSinrD2D instead of letting them fall into the uplink statistic.
     *
     * @param dir direction of the reception
     * @param ueId the UE end of the link (the sender, for an uplink reception)
     * @param carrierFrequency carrier the frame arrived on
     * @param sinr mean SINR over the resource blocks actually used
     */
    virtual void emitRcvdSinr(Direction dir, MacNodeId ueId, GHz carrierFrequency, double sinr);

    /*
     * The antenna gain of a node's radio, or gainIfNoRadio if the node is not
     * a radio on the medium: a background cell or UE, whose link budget is
     * still the model's own parameters.
     */
    double antennaGainOf(MacNodeId nodeId, double gainIfNoRadio) const;

    /*
     * The cable loss of a node's radio as a receiver, or the model's own
     * cableLoss if the node is not a radio on the medium.
     */
    double cableLossOf(MacNodeId nodeId) const;

    /*
     * Fill den[] with the per-band interference-plus-noise denominator, in dBm,
     * for the bands this frame actually uses. Computes the cellular contributions
     * (multi-cell, background-cell, external-cell). A subclass substitutes its own
     * for link types the cellular model does not describe.
     *
     * @param totN linearized thermal noise + noise figure (mW)
     */
    virtual void computeInterferencePlusNoise(const RadioLink& link, UserControlInfo *lteInfo,
            RbMap& rbmap, double totN, std::vector<double>& den);

    /*
     * Computes the per-band SINR used by isReceptionSuccessful(). Split out so that
     * a subclass can route link types the cellular model does not describe -- the
     * D2D channel model overrides this to send D2D/D2D_MULTI through getSINR_D2D.
     *
     * @param rsrpVector the RSRP the caller already holds, when it has one
     */
    virtual std::vector<double> getReceptionSinr(AirFrame *frame, UserControlInfo *lteInfo,
            const std::vector<double>& rsrpVector) { return getSINR(frame, lteInfo); }

    /*
     * Returns the 2D distance between two coordinates (ignore z-axis)
     */
    virtual double getTwoDimDistance(inet::Coord a, inet::Coord b);

    /*
     * One interfering uplink transmitter: a real UE (with a PHY) or a
     * background UE (with a traffic generator).
     */
    struct InterfererInfo
    {
        MacNodeId nodeId;
        MacCellId cellId;
        Direction dir;
        double txPwr;
        inet::Coord coord;
    };

    /*
     * The properties of an interfering transmission on the radio medium every
     * interference computation needs, its position the one it started at.
     * Shared by the uplink and D2D interference loops, which otherwise differ
     * in their exclusion rules and antenna-gain terms.
     */
    static InterfererInfo describeInterferer(const CellularTransmission& transmission);

    /*
     * The bands below numBands the transmission occupies that a reception with
     * the given RB map uses (every band, if the RB map is empty), in increasing
     * order.
     */
    static std::vector<unsigned int> sharedBands(const CellularTransmission& transmission, unsigned int numBands, const RbMap& rbmap);

    /* How long a slot of the carrier's numerology lasts. */
    simtime_t slotDuration(GHz carrierFrequency);

    /*
     * When the slot completed last on the carrier ended: the latest slot
     * boundary of the carrier's numerology not after now. A CQI measures the
     * interference of that slot.
     */
    simtime_t lastCompletedSlotEnd(GHz carrierFrequency);

    /*
     * Compute the interference at a UE in DL from the downlink data
     * transmissions of the slot (a reception's just completed one, on its
     * bands; a CQI's completed last, on every band) in creation order: the
     * other cells' if cells is set, the external and background cells' as
     * their switches say.
     * @param eNbId id of the UE's cell
     * @param isCqi if we are computing a CQI
     * @param link the link the interference is computed for, whose LOS state
     *        the path loss from external and background cells takes
     */
    virtual bool computeDownlinkInterference(MacNodeId eNbId, MacNodeId ueId, inet::Coord coord, bool isCqi, GHz carrierFrequency, const RbMap& rbmap, std::vector<double> *interference, const LinkKey& link, bool cells);

    /*
     * Whether, for a cell with one carrier, the bands of a reception with the
     * given RB map that the cell's DL data transmissions of the slot just
     * completed (from slotStart to now) occupied are those its scheduler
     * allocated. True for the
     * receiving UE's own cell and for any other cell. A debug check of the
     * radio medium against the MAC.
     */
    bool dlOccupancyMatchesScheduler(MacNodeId id, MacNodeId eNbId, GHz carrierFrequency, simtime_t slotStart, const RbMap& rbmap);

    /*
     * Compute the interference at a base station, at enbCoord, in UL from the
     * uplink transmissions of the slot (as for computeDownlinkInterference()) in
     * creation order: the UEs of other cells if cells is set, the background
     * cells' UEs as their switch says.
     */
    virtual bool computeUplinkInterference(MacNodeId eNbId, MacNodeId senderId, inet::Coord enbCoord, bool isCqi, GHz carrierFrequency, const RbMap& rbmap, std::vector<double> *interference, const LinkKey& link, bool cells);

    /*
     * The power, in dBm, with which a transmission of an external or a
     * background cell, or of a background cell's UE, arrives at the given
     * position: the model's path loss in the LOS state of the given link (see
     * computePhantomPathLoss), the antenna's attenuation, the model's own gains
     * and cable loss. 'ue' is the receiving UE of a downlink transmission, whose
     * building penetration applies; nullptr for an uplink one.
     */
    double phantomPowerAt(const CellularTransmission& transmission, const inet::Coord& position, const LinkKey& link, IRadioEndpoint *ue);

    /*
     * The path loss of a phantom transmission, in dB: the model's own path-loss
     * formulas, in the LOS state of the given link (RadioLink::linkKey) unless
     * enableExtCellLos is off
     */
    virtual double computePhantomPathLoss(double d3D, double d2D, const LinkKey& key);
};

} //namespace

#endif /* STACK_PHY_CHANNELMODEL_STOCHASTICCHANNELMODEL_H_ */

