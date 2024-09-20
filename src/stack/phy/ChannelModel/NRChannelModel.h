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

#ifndef NRCHANNELMODEL_H_
#define NRCHANNELMODEL_H_

#include "stack/phy/ChannelModel/LteRealisticChannelModel.h"

namespace simu5g {

class NRChannelModel : public LteRealisticChannelModel
{

  public:
    void initialize(int stage) override;

    /*
     * Compute attenuation caused by path loss and shadowing (optional)
     *
     * @param nodeId MAC node ID of UE
     * @param dir traffic direction
     * @param coord position of end point communication (if dir==UL it is the position of UE else it is the position of gNodeB)
     */
    double getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord coord, bool cqiDl) override;

    /*
     *  Compute attenuation caused by transmission direction
     *
     * @param angle angle
     */
    double computeAngularAttenuation(double hAngle, double vAngle) override;

    /*
     * Compute LOS probability (taken from TR 36.873)
     *
     * @param d distance between UE and gNodeB
     * @param nodeId MAC node ID of UE
     */
    void computeLosProbability(double d, MacNodeId nodeId);

    /*
     * Compute the path-loss attenuation according to the selected scenario
     *
     * @param threeDimDistance distance between UE and gNodeB (3D)
     * @param twoDimDistance distance between UE and gNodeB (2D)
     * @param los line-of-sight flag
     */
    double computePathLoss(double threeDimDistance, double twoDimDistance, bool los) override;

    /*
     * 3D-InH path loss model (taken from TR 36.873)
     *
     * @param threeDimDistance distance between UE and gNodeB
     * @param los line-of-sight flag
     */
    double computeIndoor(double threeDimDistance, double twoDimDistance, bool los);

    /*
     * 3D-UMi path loss model (taken from TR 36.873)
     *
     * @param threeDimDistance distance between UE and gNodeB
     * @param los line-of-sight flag
     */
    double computeUrbanMicro(double threeDimDistance, double twoDimDistance, bool los);

    /*
     * 3D-UMa path loss model (taken from TR 36.873)
     *
     * @param threeDimDistance distance between UE and gNodeB
     * @param los line-of-sight flag
     */
    double computeUrbanMacro(double threeDimDistance, double twoDimDistance, bool los);

    /*
     * 3D-RMa path loss model (taken from TR 36.873)
     *
     * @param threeDimDistance distance between UE and gNodeB
     * @param los line-of-sight flag
     */
    double computeRuralMacro(double threeDimDistance, double twoDimDistance, bool los);

    /*
     * Evaluates total interference from external cells seen from the spot given by coord
     * @return total interference expressed in dBm
     */
    bool computeExtCellInterference(MacNodeId eNbId, MacNodeId nodeId, inet::Coord coord, bool isCqi, double carrierFrequency, std::vector<double> *interference) override;

    /*
     * Compute attenuation due to path loss and shadowing
     * @return attenuation expressed in dBm
     */
    double computeExtCellPathLoss(double threeDimDistance, double twoDimDistance, MacNodeId nodeId);
};

} //namespace

#endif /* NRCHANNELMODEL_H_ */

