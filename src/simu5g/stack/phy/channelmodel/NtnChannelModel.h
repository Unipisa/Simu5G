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
  protected:
    inet::Coord lastTerrestrialEndpointCoord_;
    inet::Coord lastSatelliteEndpointCoord_;
    inet::GeoCoord lastTerrestrialEndpointWgs84_ = inet::GeoCoord::NIL;
    inet::Coord lastSatelliteEndpointEcefCoord_;
    std::map<MacNodeId, double> buildingPenetrationProbabilityMap_;

    double computePathLoss(double distance, double dbp, bool los) override;
    double computeShadowing(double distance, MacNodeId nodeId, double speed, bool cqiDl) override;
    double computeBuildingPenetrationLoss(MacNodeId nodeId);
    double computeAtmosphericLoss();
    double computeScintillationLoss();
    double computeFastFading(MacNodeId nodeId, double speed, bool cqiDl);
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
