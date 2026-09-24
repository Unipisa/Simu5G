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
 * - the antenna pattern attenuation and the link budget (antenna gains, cable
 *   loss, noise figures, thermal noise);
 * - interference from other cells -- downlink, uplink, external cells and
 *   background cells;
 * - the assembly of all of the above into a per-band SINR, and the mapping of
 *   that SINR onto a block error probability (with HARQ reduction) that decides
 *   reception;
 * - the per-node mobility state the correlated quantities need: position
 *   history, speed, and the point at which shadowing and LOS were last drawn;
 * - the SINR statistics.
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

    // true if the UE is inside a building
    bool inside_building_;

    // distance from the building wall
    double inside_distance_;

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

    typedef std::pair<inet::simtime_t, inet::Coord> Position;

    // Last position of current user
    std::map<MacNodeId, std::queue<Position>> positionHistory_;

    // Per link: the position at which the LOS probability was last computed.
    std::map<LinkKey, Position> lastCorrelationPoint_;

    // Scenario
    DeploymentScenario scenario_;

    // Formulas of the selected 3GPP propagation study; owned, created in initialize()
    PathLossModel *pathLoss_ = nullptr;

    // The ext-cell and background-cell interference paths evaluate their path
    // loss with the TR 36.814 formulas regardless of which propagation study
    // the model uses for its own links (see computeExtCellPathLoss); owned
    PathLossModel *extCellPathLoss_ = nullptr;

    // Per link: whether it is in Line of Sight
    std::map<LinkKey, bool> losMap_;

    // Stores the last computed shadowing for each user
    typedef std::map<LinkKey, std::pair<inet::simtime_t, double>> ShadowFadingMap;
    ShadowFadingMap lastComputedSF_;

    // Correlation distance used in shadowing computation and
    // also used to recompute the probability of LOS
    double correlationDistance_;

    // Percentage of error probability reduction for each h-arq retransmission
    double harqReduction_;

    // Antenna gain of eNodeB
    double antennaGainEnB_;

    // Antenna gain of micro node
    double antennaGainMicro_;

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

    // Struct used to store information about Jakes fading
    struct JakesFadingData
    {
        std::vector<double> angleOfArrival;
        std::vector<simtime_t> delaySpread;

        friend std::ostream& operator<<(std::ostream& os, const JakesFadingData& e)
        {
            os << "paths=" << e.angleOfArrival.size();
            return os;
        }
    };

    // For each node and for each band we store information about Jakes fading
    std::map<LinkKey, std::vector<JakesFadingData>> jakesFadingMap_;

    // For each node and for each band we store information about Jakes fading
    std::map<LinkKey, std::vector<JakesFadingData>> jakesFadingMapBgUe_;

    typedef std::vector<JakesFadingData> JakesFadingVector;
    typedef std::map<LinkKey, JakesFadingVector> JakesFadingMap;

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

    // Statistics
    static simsignal_t rcvdSinrDlSignal_;
    static simsignal_t rcvdSinrUlSignal_;
    static simsignal_t measuredSinrDlSignal_;
    static simsignal_t measuredSinrUlSignal_;

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
    double getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord coord, bool cqiDl)
    {
        return getAttenuation(cellularLink(nodeId, dir, coord, cqiDl));
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
     * Compute shadowing
     *
     * @param d3D 3D distance between UE and eNodeB
     * @param d2D 2D distance between UE and eNodeB
     * @param nodeid mac node id of UE
     * @param speed speed of UE
     */
    virtual double computeShadowing(double d3D, double d2D, const LinkKey& key, MacNodeId ownerId, double speed, bool cqiDl);

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
    std::vector<double> getSINR_bgUe(AirFrame *frame, UserControlInfo *lteInfo) override;

    /*
     * Compute received power for a background UE according to pathloss
     *
     */
    double getReceivedPower_bgUe(double txPower, inet::Coord txPos, inet::Coord rxPos, Direction dir, bool losStatus, MacNodeId bsId) override;

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
     * Compute Jakes fading
     *
     * @param speed speed of UE
     * @param nodeid mac node id of UE
     * @param band logical band id
     * @param cqiDl if true, the jakesMap in the UE side should be used
     * @param isBgUe if true, this is called for a background UE
     */
    virtual double jakesFading(const LinkKey& key, MacNodeId ownerId, double speed, unsigned int band, bool cqiDl, bool isBgUe = false);

    /*
     * Compute LOS probability
     *
     * @param d3D 3D distance between UE and eNodeB
     * @param d2D 2D distance between UE and eNodeB
     * @param nodeid mac node id of UE
     */
    virtual void computeLosProbability(double d3D, double d2D, const LinkKey& key);

    JakesFadingMap *getJakesMap()
    {
        return &jakesFadingMap_;
    }

    ShadowFadingMap *getShadowingMap()
    {
        return &lastComputedSF_;
    }

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
     * Build the RadioLink described by a frame's control info (DL, UL, and the
     * feedback variants). The D2D path builds its links separately -- its API
     * takes the peer endpoint explicitly rather than deriving it.
     */
    virtual RadioLink linkFor(UserControlInfo *lteInfo);

    /*
     * Build the RadioLink for a UE<->serving-BS link expressed the old way: the
     * local module is one endpoint, 'coord' the other, and 'dir' says which of
     * the two is the UE.
     */
    RadioLink cellularLink(MacNodeId ueId, Direction dir, inet::Coord coord, bool cqiDl);

    /*
     * Emit the received-SINR statistic for a decoded frame. Routes D2D/D2D_MULTI
     * receptions to rcvdSinrD2D instead of letting them fall into the uplink
     * statistic, which is where the DL/else split used to put them.
     *
     * @param dir direction of the reception
     * @param ueId the UE end of the link (the sender, for an uplink reception)
     * @param carrierFrequency carrier the frame arrived on
     * @param sinr mean SINR over the resource blocks actually used
     */
    virtual void emitRcvdSinr(Direction dir, MacNodeId ueId, GHz carrierFrequency, double sinr);

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
     * Compute speed (m/s) for a given node
     * @param nodeid mac node id of UE
     * @return the speed in m/s
     */
    virtual double computeSpeed(const MacNodeId nodeId, const inet::Coord coord);

    /*
     * Compute the euclidean distance between the current position and the
     * last position used to calculate the LOS probability
     */
    virtual double computeCorrelationDistance(const LinkKey& key, const inet::Coord coord);

    /*
     * Update base point if distance to previous value is greater than the
     * correlationDistance_
     */
    virtual void updateCorrelationDistance(const LinkKey& key, const inet::Coord coord);

    /*
     * Updates position for a given node
     * @param nodeid mac node id of UE
     */
    virtual void updatePositionHistory(const MacNodeId nodeId, const inet::Coord coord);

    /*
     * One interfering transmitter, normalized across the two kinds the UL
     * transmission map can hold: a real UE (with a PHY) and a background UE
     * (with a traffic generator).
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
     * Unpack a UeAllocationInfo into the properties every interference
     * computation needs. Shared by the uplink and D2D interference loops, which
     * otherwise differ in their exclusion rules and antenna-gain terms.
     */
    static InterfererInfo describeInterferer(const UeAllocationInfo& allocation);

    /*
     * Compute total interference due to eNB coexistence for the DL direction
     * @param eNbId id of the considered eNb
     * @param isCqi if we are computing a CQI
     */
    virtual bool computeDownlinkInterference(MacNodeId eNbId, MacNodeId ueId, inet::Coord coord, bool isCqi, GHz carrierFrequency, const RbMap& rbmap, std::vector<double> *interference);

    /*
     * Compute interference coming from neighboring cells for the UL direction
     */
    virtual bool computeUplinkInterference(MacNodeId eNbId, MacNodeId senderId, bool isCqi, GHz carrierFrequency, const RbMap& rbmap, std::vector<double> *interference);

    /*
     * Evaluates total interference from external cells seen from the spot given by coord
     * @return total interference expressed in dBm
     */
    virtual bool computeExtCellInterference(MacNodeId eNbId, MacNodeId nodeId, inet::Coord coord, bool isCqi, GHz carrierFrequency, std::vector<double> *interference);

    /*
     * Evaluates total interference from external cells seen from the spot given by coord
     * @return total interference expressed in dBm
     */
    virtual bool computeBackgroundCellInterference(MacNodeId nodeId, inet::Coord bsCoord, inet::Coord ueCoord, bool isCqi, GHz carrierFrequency, const RbMap& rbmap, Direction dir, std::vector<double> *interference);

    /*
     * Compute attenuation due to path loss and shadowing
     * @return attenuation expressed in dBm
     */
    virtual double computeExtCellPathLoss(double dist, const LinkKey& key);

    /*
     * The channel model of the specified UE on this model's carrier. Throws if
     * the Binder does not know the UE, or if that model is not a
     * StochasticChannelModel.
     * @param id mac id of the user
     */
    virtual StochasticChannelModel *obtainUeChannelModel(MacNodeId id);

    /*
     * Obtain the jakes map for the specified UE
     * @param id mac id of the user
     */
    virtual JakesFadingMap *obtainUeJakesMap(MacNodeId id);

    /*
     * Obtain the shadowing map for the specified UE
     * @param id mac id of the user
     */
    virtual ShadowFadingMap *obtainShadowingMap(MacNodeId id);
};

} //namespace

#endif /* STACK_PHY_CHANNELMODEL_STOCHASTICCHANNELMODEL_H_ */

