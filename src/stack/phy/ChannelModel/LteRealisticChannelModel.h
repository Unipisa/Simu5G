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

#ifndef STACK_PHY_CHANNELMODEL_LTEREALISTICCHANNELMODEL_H_
#define STACK_PHY_CHANNELMODEL_LTEREALISTICCHANNELMODEL_H_

#include <omnetpp.h>
#include "stack/phy/ChannelModel/LteChannelModel.h"

namespace simu5g {

using namespace omnetpp;

class Binder;

class LteRealisticChannelModel : public LteChannelModel
{
  protected:

    // Information needed about the playground
    bool useTorus_;

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
    bool enableD2DInterference_;

    bool enable_extCell_los_;

    typedef std::pair<inet::simtime_t, inet::Coord> Position;

    // Last position of current user
    std::map<MacNodeId, std::queue<Position>> positionHistory_;

    // Last position of current user at which probability of LOS was computed.
    std::map<MacNodeId, Position> lastCorrelationPoint_;

    // Scenario
    DeploymentScenario scenario_;

    // Map that stores for each user if is in Line of Sight or not with eNodeB
    std::map<MacNodeId, bool> losMap_;

    // Stores the last computed shadowing for each user
    typedef std::map<MacNodeId, std::pair<inet::simtime_t, double>> ShadowFadingMap;
    ShadowFadingMap lastComputedSF_;

    // Correlation distance used in shadowing computation and
    // also used to recompute the probability of LOS
    double correlationDistance_;

    // Percentage of error probability reduction for each h-arq retransmission
    double harqReduction_;

    // Eigen values of channel matrix, used to compute the rank
    double lambdaMinTh_;
    double lambdaMaxTh_;
    double lambdaRatioTh_;

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
    };

    // For each node and for each band we store information about Jakes fading
    std::map<MacNodeId, std::vector<JakesFadingData>> jakesFadingMap_;

    // For each node and for each band we store information about Jakes fading
    std::map<MacNodeId, std::vector<JakesFadingData>> jakesFadingMapBgUe_;

    typedef std::vector<JakesFadingData> JakesFadingVector;
    typedef std::map<MacNodeId, JakesFadingVector> JakesFadingMap;

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
    static simsignal_t rcvdSinrD2DSignal_;
    static simsignal_t measuredSinrDlSignal_;
    static simsignal_t measuredSinrUlSignal_;

  public:
    void initialize(int stage) override;

    /*
     * Compute Attenuation caused by pathloss and shadowing (optional)
     *
     * @param nodeid mac node id of UE
     * @param dir traffic direction
     * @param coord position of end point communication (if dir==UL is the position of UE else is the position of eNodeB)
     */
    double getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord coord, bool cqiDl) override;

    /*
     * Compute Attenuation for D2D caused by pathloss and shadowing (optional)
     *
     * @param nodeid mac node id of UE
     * @param dir traffic direction
     * @param coord position of end point communication (if dir==UL is the position of UE else is the position of eNodeB)
     */
    virtual double getAttenuation_D2D(MacNodeId nodeId, Direction dir, inet::Coord coord, MacNodeId node2_Id, inet::Coord coord_2, bool cqiDl);

    /*
     *  Compute angle between two coordinates
     *
     * @param center first coord
     * @param point second coord
     */
    double computeAngle(Coord center, Coord point);

    /*
     *  Compute vertical angle between two coordinates
     *  The returned angle is the angle with respect to the zenith direction
     *
     * @param center first coord
     * @param point second coord
     * @return angle
     */
    double computeVerticalAngle(Coord center, Coord point);

    /*
     *  Compute Attenuation caused by transmission direction
     *
     * @param angle angle
     */
    virtual double computeAngularAttenuation(double hAngle, double vAngle = 0);

    /*
     * Compute shadowing
     *
     * @param distance between UE and eNodeB
     * @param nodeid mac node id of UE
     * @param speed speed of UE
     */
    virtual double computeShadowing(double sqrDistance, MacNodeId nodeId, double speed, bool cqiDl);

    /*
     * Compute sir for each band for user nodeId according to multipath fading
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    std::vector<double> getSIR(LteAirFrame *frame, UserControlInfo *lteInfo) override;

    /*
     * Compute sinr for each band for user nodeId according to pathloss, shadowing (optional) and multipath fading
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    std::vector<double> getSINR(LteAirFrame *frame, UserControlInfo *lteInfo) override;

    /*
     * Compute received useful signal for each band for user nodeId according to pathloss, shadowing (optional) and multipath fading
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    std::vector<double> getRSRP(LteAirFrame *frame, UserControlInfo *lteInfo) override;

    /*
     * Compute sinr for each band for a background UE according to pathloss
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    std::vector<double> getSINR_bgUe(LteAirFrame *frame, UserControlInfo *lteInfo) override;

    /*
     * Compute received power for a background UE according to pathloss
     *
     */
    double getReceivedPower_bgUe(double txPower, inet::Coord txPos, inet::Coord rxPos, Direction dir, bool losStatus, MacNodeId bsId) override;

    /*
     * Compute Received useful signal for D2D transmissions
     */
    std::vector<double> getRSRP_D2D(LteAirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, inet::Coord destCoord) override;

    /*
     * Compute sinr (D2D) for each band for user nodeId according to pathloss, shadowing (optional) and multipath fading
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    std::vector<double> getSINR_D2D(LteAirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, inet::Coord destCoord, MacNodeId enbId) override;
    std::vector<double> getSINR_D2D(LteAirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, inet::Coord destCoord, MacNodeId enbId, const std::vector<double>& rsrpVector) override;

    /*
     * Compute the error probability of the transmitted packet according to cqi used, txmode, and the received power
     * after that it throws a random number in order to check if this packet will be corrupted or not
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     * @param rsrpVector the received signal for each RB, if it has already been computed
     */
    bool isError_D2D(LteAirFrame *frame, UserControlInfo *lteI, const std::vector<double>& rsrpVector) override;

    /*
     * Compute the error probability of the transmitted packet according to cqi used, txmode, and the received power
     * after that it throws a random number in order to check if this packet will be corrupted or not
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    bool isError(LteAirFrame *frame, UserControlInfo *lteI) override;

    /*
     * The same as before but used for das TODO to be implemented
     *
     * @param frame pointer to the packet
     * @param lteinfo pointer to the user control info
     */
    bool isErrorDas(LteAirFrame *frame, UserControlInfo *lteI) override
    {
        throw cRuntimeError("DAS PHY LAYER TO BE IMPLEMENTED");
        return -1;
    }

    /*
     * Compute the path-loss attenuation according to the selected scenario
     *
     * @param distance between UE and eNodeB
     * @param los line-of-sight flag
     */
    double computePathLoss(double distance, double dbp, bool los) override;

    /*
     * Compute attenuation for indoor scenario
     *
     * @param distance between UE and eNodeB
     * @param los line-of-sight flag
     */
    double computeIndoor(double distance, bool los);

    /*
     * Compute attenuation for Urban Micro cell
     *
     * @param distance between UE and eNodeB
     * @param los line-of-sight flag
     */
    double computeUrbanMicro(double distance, bool los);

    /*
     * Compute attenuation for Urban Macro cell
     *
     * @param distance between UE and eNodeB
     * @param los line-of-sight flag
     */
    double computeUrbanMacro(double distance, bool los);

    /*
     * Compute attenuation for Sub Urban Macro cell
     *
     * @param distance between UE and eNodeB
     * @param los line-of-sight flag
     */
    double computeSubUrbanMacro(double distance, double& dbp, bool los);

    /*
     * Compute attenuation for Rural Macro cell
     *
     * @param distance between UE and eNodeB
     * @param los line-of-sight flag
     */
    double computeRuralMacro(double distance, double& dbp, bool los);

    /*
     * Compute std deviation of shadowing according to scenario and visibility
     *
     * @param distance between UE and eNodeB
     * @param nodeid mac node id of UE
     */
    double getStdDev(bool dist, MacNodeId nodeId);

    /*
     * Compute Rayleigh fading
     *
     * @param i index in the trace file
     * @param nodeid mac node id of UE
     */
    double rayleighFading(MacNodeId id, unsigned int band);

    /*
     * Compute Jakes fading
     *
     * @param speed speed of UE
     * @param nodeid mac node id of UE
     * @param band logical band id
     * @param cqiDl if true, the jakesMap in the UE side should be used
     * @param isBgUe if true, this is called for a background UE
     */
    double jakesFading(MacNodeId nodeId, double speed, unsigned int band, bool cqiDl, bool isBgUe = false);

    /*
     * Compute LOS probability
     *
     * @param d between UE and eNodeB
     * @param nodeid mac node id of UE
     */
    void computeLosProbability(double d, MacNodeId nodeId);

    JakesFadingMap *getJakesMap()
    {
        return &jakesFadingMap_;
    }

    ShadowFadingMap *getShadowingMap()
    {
        return &lastComputedSF_;
    }

    bool isUplinkInterferenceEnabled() override { return enableUplinkInterference_; }
    bool isD2DInterferenceEnabled() override { return enableD2DInterference_; }

  protected:

    /*
     * Returns the 2D distance between two coordinates (ignore z-axis)
     */
    double getTwoDimDistance(inet::Coord a, inet::Coord b);

    /*
     * Compute speed (m/s) for a given node
     * @param nodeid mac node id of UE
     * @return the speed in m/s
     */
    double computeSpeed(const MacNodeId nodeId, const inet::Coord coord);

    /*
     * Compute the euclidean distance between the current position and the
     * last position used to calculate the LOS probability
     */
    double computeCorrelationDistance(const MacNodeId nodeId, const inet::Coord coord);

    /*
     * Update base point if distance to previous value is greater than the
     * correlationDistance_
     */
    void updateCorrelationDistance(const MacNodeId nodeId, const inet::Coord coord);

    /*
     * Updates position for a given node
     * @param nodeid mac node id of UE
     */
    void updatePositionHistory(const MacNodeId nodeId, const inet::Coord coord);

    /*
     * Compute total interference due to eNB coexistence for the DL direction
     * @param eNbId id of the considered eNb
     * @param isCqi if we are computing a CQI
     */
    bool computeDownlinkInterference(MacNodeId eNbId, MacNodeId ueId, inet::Coord coord, bool isCqi, double carrierFrequency, const RbMap& rbmap, std::vector<double> *interference);

    /*
     * Compute interference coming from neighboring cells for the UL direction
     */
    bool computeUplinkInterference(MacNodeId eNbId, MacNodeId senderId, bool isCqi, double carrierFrequency, const RbMap& rbmap, std::vector<double> *interference);

    /*
     * Compute interference coming from neighboring UEs for the D2D/D2D_MULTI direction
     */
    bool computeD2DInterference(MacNodeId eNbId, MacNodeId senderId, inet::Coord senderCoord, MacNodeId destId, inet::Coord destCoord, bool isCqi, double carrierFrequency, const RbMap& rbmap, std::vector<double> *interference, Direction dir);

    /*
     * Evaluates total interference from external cells seen from the spot given by coord
     * @return total interference expressed in dBm
     */
    virtual bool computeExtCellInterference(MacNodeId eNbId, MacNodeId nodeId, inet::Coord coord, bool isCqi, double carrierFrequency, std::vector<double> *interference);

    /*
     * Evaluates total interference from external cells seen from the spot given by coord
     * @return total interference expressed in dBm
     */
    virtual bool computeBackgroundCellInterference(MacNodeId nodeId, inet::Coord bsCoord, inet::Coord ueCoord, bool isCqi, double carrierFrequency, const RbMap& rbmap, Direction dir, std::vector<double> *interference);

    /*
     * Compute attenuation due to path loss and shadowing
     * @return attenuation expressed in dBm
     */
    double computeExtCellPathLoss(double dist, MacNodeId nodeId);

    /*
     * Obtain the jakes map for the specified UE
     * @param id mac id of the user
     */
    JakesFadingMap *obtainUeJakesMap(MacNodeId id);
    JakesFadingMap *obtainUeJakesMap_bgUe(MacNodeId id);

    /*
     * Obtain the shadowing map for the specified UE
     * @param id mac id of the user
     */
    ShadowFadingMap *obtainShadowingMap(MacNodeId id);
};

} //namespace

#endif /* STACK_PHY_CHANNELMODEL_LTEREALISTICCHANNELMODEL_H_ */

