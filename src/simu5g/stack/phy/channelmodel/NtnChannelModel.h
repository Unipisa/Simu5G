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

#include <array>
#include <vector>

#include "inet/common/geometry/common/GeographicCoordinateSystem.h"
#include "simu5g/stack/phy/channelmodel/LteRealisticChannelModel.h"
#include "simu5g/stack/phy/channelmodel/NtnChannelModelTables.h"
#include "simu5g/stack/phy/antennamodel/IAntennaModel.h"

namespace simu5g {

class NtnChannelModel : public LteRealisticChannelModel
{
  protected:

    inet::Coord lastTerrestrialEndpointCoord_;
    inet::Coord lastSatelliteEndpointCoord_;
    inet::GeoCoord lastTerrestrialEndpointWgs84_ = inet::GeoCoord::NIL;
    inet::Coord lastSatelliteEndpointEcefCoord_;
    std::map<MacNodeId, double> buildingPenetrationProbabilityMap_;
    double polarizationMismatchLoss_ = 0.0; // in dB

    inet::ModuleRefByPar<IAntennaModel> antennaModel_;

    /*
     * Frequency-selective fading data structures
     */

    struct FrequencySelectiveFadingState
    {
        bool initialized = false;
        bool los = false;
        bool isSband = false;
        int angleIndex = 0;
        DeploymentScenario scenario = UNKNOWN_SCENARIO;
        simtime_t referenceTime = SIMTIME_ZERO;
        double refreshDistance = 0.0;
        double losLinearPower = 0.0;
        double losDopplerHz = 0.0;
        double losInitialPhaseRad = 0.0;
        inet::Coord lastRefreshCoord;
        std::vector<double> delaysSeconds;
        std::vector<double> linearPowers;
        std::vector<double> dopplerHz;
        std::vector<double> initialPhaseRad;
    };

    std::map<MacNodeId, FrequencySelectiveFadingState> fadingStateMap_;

    std::vector<double> computeReceptionSinr(LteAirFrame *frame, UserControlInfo *lteInfo) override;

    // compute Line-of-Sight probability and store it into the specific los map
    void computeLosProbability(double d, MacNodeId nodeId);

    // compute large scale attenuation (path loss, clutter loss, penetration loss, atmospheric, scintillation)
    double getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord coord, bool cqiDl) override;

    /*
     * Large-scale model methods
     */
    double computePathLoss(double distance, double dbp, bool los) override;
    double computeShadowing(double distance, MacNodeId nodeId, double speed, bool cqiDl) override;
    double computeBuildingPenetrationLoss(MacNodeId nodeId);
    double computeAtmosphericLoss();
    double computeScintillationLoss();

    /*
     * Frequency-selective fading methods
     */

    std::vector<double> computeFrequencySelectiveFading(MacNodeId nodeId, double speed);
    const FrequencySelectiveScenarioParameters& getFrequencySelectiveScenarioParameters(bool los, bool isSband) const;
    void initializeFrequencySelectiveFadingState(FrequencySelectiveFadingState& state, const FrequencySelectiveScenarioParameters& params, int angleIndex, double maxDopplerHz);
    double getFadingRefreshDistance(bool los) const;
    std::vector<double> getBandCenterFrequenciesHz() const;

  public:
    void initialize(int stage) override;

    std::vector<double> getSINR(LteAirFrame *frame, UserControlInfo *lteInfo) override;
    std::vector<double> getRSRP(LteAirFrame *frame, UserControlInfo *lteInfo) override;
};

} // namespace simu5g

#endif /* NTNCHANNELMODEL_H_ */
