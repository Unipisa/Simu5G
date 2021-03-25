//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef NRCHANNELMODEL_3GPP38_901_H_
#define NRCHANNELMODEL_3GPP38_901_H_

#include "stack/phy/ChannelModel/NRChannelModel.h"

class NRChannelModel_3GPP38_901 : public NRChannelModel
{

public:
    virtual void initialize(int stage);

    /*
     * Compute LOS probability (taken from TR 36.873)
     *
     * @param d between UE and eNodeB
     * @param nodeid mac node id of UE
     */
    void computeLosProbability(double d, MacNodeId nodeId);

    /*
     * Compute the path-loss attenuation according to the selected scenario
     *
     * @param 3d_distance between UE and eNodeB (3D)
     * @param 2d_distance between UE and eNodeB (2D)
     * @param los line-of-sight flag
     */
    virtual double computePathLoss(double threeDimDistance, double twoDimDistance, bool los);

    /*
     * 3D-UMa path loss model (taken from TR 36.873)
     *
     * @param distance between UE and eNodeB
     * @param los line-of-sight flag
     */
    double computeUrbanMacro(double threeDimDistance, double twoDimDistance, bool los);

    /*
     * compute std deviation of shadowing according to scenario and visibility
     *
     * @param distance between UE and eNodeB
     * @param nodeid mac node id of UE
     */
    double getStdDev(bool dist, MacNodeId nodeId);

    /*
     * Compute shadowing
     *
     * @param distance between UE and eNodeB
     * @param nodeid mac node id of UE
     * @param speed speed of UE
     */
    virtual double computeShadowing(double sqrDistance, MacNodeId nodeId, double speed, bool cqiDl);

};

#endif /* NRChannelModel_3GPP38_901_H_ */
