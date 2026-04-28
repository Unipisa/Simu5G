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

#ifndef NTNCHANNELMODEL_H_
#define NTNCHANNELMODEL_H_

#include "inet/common/geometry/common/GeographicCoordinateSystem.h"
#include "simu5g/stack/phy/channelmodel/LteRealisticChannelModel.h"

namespace simu5g {

class NtnChannelModel : public LteRealisticChannelModel
{
  public:
    enum class LmsStateType
    {
        GOOD,
        BAD
    };

    struct LmsStateParameters
    {
        double muDuration;
        double sigmaDuration;
        double minDuration;
        double muMa;
        double sigmaMa;
        double h1;
        double h2;
        double g1;
        double g2;
        double correlationDistance;
        double transitionSlope;
        double transitionOffset;
    };

    struct LmsScenarioParameters
    {
        LmsStateParameters good;
        LmsStateParameters bad;
        double pBadMin;
        double pBadMax;
    };

    struct LmsFadingState
    {
        bool initialized = false;
        bool inTransition = false;
        LmsStateType state = LmsStateType::GOOD;
        double remainingDistance = 0.0;
        double currentMa = 0.0;
        double currentSigmaA = 0.0;
        double currentMp = 0.0;
        double currentCorrelationDistance = 1.0;
        double directPathAmplitudeDb = 0.0;
        double transitionLength = 0.0;
        double transitionProgress = 0.0;
        double sourceMa = 0.0;
        double targetMa = 0.0;
        double sourceSigmaA = 0.0;
        double targetSigmaA = 0.0;
        double sourceMp = 0.0;
        double targetMp = 0.0;
        double sourceCorrelationDistance = 1.0;
        double targetCorrelationDistance = 1.0;
        inet::Coord lastCoord;
        bool hasLastCoord = false;
    };

  protected:
    bool useTwoStateModelFading_ = false;
    inet::Coord lastTerrestrialEndpointCoord_;
    inet::Coord lastSatelliteEndpointCoord_;
    inet::GeoCoord lastTerrestrialEndpointWgs84_ = inet::GeoCoord::NIL;
    inet::Coord lastSatelliteEndpointEcefCoord_;
    std::map<MacNodeId, double> buildingPenetrationProbabilityMap_;
    std::map<MacNodeId, LmsFadingState> lmsFadingStateMap_;

    double computePathLoss(double distance, double dbp, bool los) override;
    double computeShadowing(double distance, MacNodeId nodeId, double speed, bool cqiDl) override;
    double computeBuildingPenetrationLoss(MacNodeId nodeId);
    double computeAtmosphericLoss();
    double computeScintillationLoss();
    double computeFastFading(MacNodeId nodeId, double speed, bool cqiDl);
    const LmsScenarioParameters& getLmsScenarioParameters(double elevationAngle) const;
    double sampleLmsDuration(const LmsStateParameters& params);
    double sampleLmsMeanAmplitude(const LmsStateParameters& params, bool isBadState, double pBadMin, double pBadMax);
    void initializeLmsFadingState(LmsFadingState& state, const LmsScenarioParameters& params);
    void startLmsTransition(LmsFadingState& state, const LmsScenarioParameters& params);
    void advanceLmsDirectPath(LmsFadingState& state, double distanceStep);
    void computeLosProbability(double d, MacNodeId nodeId);

  public:
    void initialize(int stage) override;

    // compute large scale attenuation (path loss, clutter loss, penetration loss, atmospheric, scintillation)
    double getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord coord, bool cqiDl) override;
    std::vector<double> getSINR(LteAirFrame *frame, UserControlInfo *lteInfo) override;
    std::vector<double> getRSRP(LteAirFrame *frame, UserControlInfo *lteInfo) override;
};

} // namespace simu5g

#endif /* NTNCHANNELMODEL_H_ */
