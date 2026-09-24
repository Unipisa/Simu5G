//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __BACKGROUNDCELLCHANNELMODEL_H_
#define __BACKGROUNDCELLCHANNELMODEL_H_

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/common/LteCommon.h"
#include "simu5g/common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

class Tr36814PathLossModel;

/**
 * This channel model implements path loss, LOS probability, and shadowing for background cells
 * according to the following 3GPP specifications:
 * - 3GPP TR 36.814, "Further advancements for E-UTRA physical layer aspects", v9.2.0, March 2017
 * - 3GPP TR 36.873, "Study on 3D channel model for LTE", v12.7.0, December 2017
 *
 * The model supports various deployment scenarios including:
 * - Indoor Hotspot (InH)
 * - Urban Microcell (UMi)
 * - Urban Macrocell (UMa)
 * - Rural Macrocell (RMa)
 * - Suburban Macrocell (SMa)
 *
 * Path loss formulas follow 3GPP specifications with proper frequency handling:
 * - Carrier frequency in Hz for physical calculations (Doppler, break-point distance)
 * - Carrier frequency in GHz for path loss formulas as specified in standards
 */
class BackgroundCellChannelModel : public cSimpleModule
{
    // carrier frequency for this cell
    GHz carrierFrequency_;
    double carrierFrequencyHz_;
    double carrierFrequencyGHz_;
    double log10CarrierFrequencyGHz_;

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

    // Average street's width
    double wStreet_;

    // scenario
    DeploymentScenario scenario_;

    //Antenna gain of eNodeB
    double antennaGainEnB_;

    //Antenna gain of micro node
    double antennaGainMicro_;

    //Antenna gain of UE
    double antennaGainUe_;

    //Thermal noise
    double thermalNoise_;

    //pointer to Binder module
    inet::ModuleRefByPar<Binder> binder_;

    //Cable loss
    double cableLoss_;

    //UE noise figure
    double ueNoiseFigure_;

    //eNodeB noise figure
    double bsNoiseFigure_;

    //if true, shadowing is enabled
    bool shadowing_;

    //Enable or disable fading
    bool fading_;

    //Number of fading paths in jakes fading
    int fadingPaths_;

    //avg delay spread in jakes fading
    double delayRMS_;

    // enable/disable intercell interference computation
    bool enableBackgroundCellInterference_;
    bool enableDownlinkInterference_;
    bool enableUplinkInterference_;

    // Store the last computed shadowing for each user
    std::map<MacNodeId, std::pair<inet::simtime_t, double>> lastComputedSF_;

    // map that stores for each user if is in Line of Sight or not with eNodeB
    std::map<MacNodeId, bool> losMap_;

    //correlation distance used in shadowing computation and
    //also used to recompute the probability of LOS
    double correlationDistance_;

    typedef std::pair<inet::simtime_t, inet::Coord> Position;

    // last position of current user
    std::map<MacNodeId, std::queue<Position>> positionHistory_;

    // last position of current user at which probability of LOS
    // was computed.
    std::map<MacNodeId, Position> lastCorrelationPoint_;

    bool tolerateMaxDistViolation_;

    //Struct used to store information about jakes fading
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

    // for each node and for each band we store information about jakes fading
    std::map<MacNodeId, std::vector<JakesFadingData>> jakesFadingMap_;

    typedef std::vector<JakesFadingData> JakesFadingVector;
    typedef std::map<MacNodeId, JakesFadingVector> JakesFadingMap;

    enum FadingType
    {
        RAYLEIGH, JAKES
    };

    //Fading type (JAKES or RAYLEIGH)
    FadingType fadingType_;

    //enable or disable the dynamic computation of LOS NLOS probability for each user
    bool dynamicLos_;

    //if dynamicLos is false this boolean is initialized to true if all users will be in LOS or false otherwise
    bool fixedLos_;

    // TR 36.814 path-loss, LOS-probability and shadowing-sigma formulas;
    // owned, created in initialize(). The main channel model's ext-cell path
    // is permanently pinned to the same study (see its extCellPathLoss_),
    // so background cells and ext cells evaluate identical formulas.
    Tr36814PathLossModel *pathLoss_ = nullptr;

    /*
     * Compute Attenuation caused by pathloss and shadowing (optional)
     */
    virtual double getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord bgBsCoord, inet::Coord bgUeCoord);
    /*
     * Compute the path-loss attenuation according to the selected scenario
     *
     * @param distance between UE and eNodeB
     * @param los line-of-sight flag
     */
    virtual double computePathLoss(double distance, double dbp, bool los);
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
    double jakesFading(MacNodeId nodeId, double speed, unsigned int band, unsigned int numBands);
    /*
     * Compute LOS probability
     *
     * @param d distance between UE and eNodeB
     * @param nodeid mac node id of UE
     */
    void computeLosProbability(double d, MacNodeId nodeId);

    JakesFadingMap *getJakesMap()
    {
        return &jakesFadingMap_;
    }

    /* compute speed (m/s) for a given node
     * @param nodeid mac node id of UE
     * @return the speed in m/s
     */
    double computeSpeed(const MacNodeId nodeId, const inet::Coord coord);

    /*
     * compute the euclidean distance between the current position and the
     * last position used to calculate the LOS probability
     */
    double computeCorrelationDistance(const MacNodeId nodeId, const inet::Coord coord);

    /*
     * update base point if the distance to the previous value is greater than the
     * correlationDistance_
     */
    void updateCorrelationDistance(const MacNodeId nodeId, const inet::Coord coord);

    /*
     * Updates position for a given node
     * @param nodeid mac node id of UE
     */
    void updatePositionHistory(const MacNodeId nodeId, const inet::Coord coord);

    double computeShadowing(double sqrDistance, MacNodeId nodeId, double speed);

    double computeAngle(inet::Coord center, inet::Coord point);
    double computeVerticalAngle(inet::Coord center, inet::Coord point);
    double getTwoDimDistance(inet::Coord a, inet::Coord b);
    double computeAngularAttenuation(double hAngle, double vAngle);

    bool computeDownlinkInterference(MacNodeId bgUeId, inet::Coord bgUePos, GHz carrierFrequency, const RbMap& rbmap, unsigned int numBands, std::vector<double> *interference);
    bool computeUplinkInterference(MacNodeId bgUeId, inet::Coord bgUePos, GHz carrierFrequency, const RbMap& rbmap, unsigned int numBands, std::vector<double> *interference);
    bool computeBackgroundCellInterference(MacNodeId bgUeId, inet::Coord bgUeCoord, int bgBsId, inet::Coord bgBsCoord, GHz carrierFrequency, const RbMap& rbmap, Direction dir, unsigned int numBands, std::vector<double> *interference);

  protected:
    void initialize(int stage) override;
    int numInitStages() const override { return inet::NUM_INIT_STAGES; }

  public:
    ~BackgroundCellChannelModel() override;

    // set carrier frequency
    void setCarrierFrequency(GHz carrierFrequency) {
        carrierFrequency_ = carrierFrequency;
        carrierFrequencyGHz_ = GHz(carrierFrequency).get();
        carrierFrequencyHz_ = Hz(carrierFrequency).get();
        log10CarrierFrequencyGHz_ = log10(carrierFrequencyGHz_);
    }

    /*
     * Compute SINR for each band for user nodeId according to pathloss, shadowing (optional) and multipath fading
     */
    virtual std::vector<double> getSINR(MacNodeId bgUeId, inet::Coord bgUePos, TrafficGeneratorBase *bgUe, BackgroundScheduler *bgScheduler, Direction dir);
};

} //namespace

#endif
