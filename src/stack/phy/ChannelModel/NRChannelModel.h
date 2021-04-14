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

class NRChannelModel : public LteRealisticChannelModel
{

public:
    virtual void initialize(int stage);

    /*
     * Compute Attenuation caused by pathloss and shadowing (optional)
     *
     * @param nodeid mac node id of UE
     * @param dir traffic direction
     * @param coord position of end point comunication (if dir==UL is the position of UE else is the position of eNodeB)
     */
    virtual double getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord coord);

    /*
    *  Compute Attenuation caused by transmission direction
    *
    * @param angle angle
    */
    virtual double computeAngolarAttenuation(double hAngle, double vAngle);

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
     * 3D-InH path loss model (taken from TR 36.873)
     *
     * @param distance between UE and eNodeB
     * @param los line-of-sight flag
     */
    double computeIndoor(double threeDimDistance, double twoDimDistance, bool los);

    /*
     * 3D-UMi path loss model (taken from TR 36.873)
     *
     * @param distance between UE and eNodeB
     * @param los line-of-sight flag
     */
    double computeUrbanMicro(double threeDimDistance, double twoDimDistance, bool los);

    /*
     * 3D-UMa path loss model (taken from TR 36.873)
     *
     * @param distance between UE and eNodeB
     * @param los line-of-sight flag
     */
    double computeUrbanMacro(double threeDimDistance, double twoDimDistance, bool los);

    /*
     * 3D-RMa path loss model (taken from TR 36.873)
     *
     * @param distance between UE and eNodeB
     * @param los line-of-sight flag
     */
    double computeRuralMacro(double threeDimDistance, double twoDimDistance, bool los);


    /*
     * evaluates total interference from external cells seen from the spot given by coord
     * @return total interference expressed in dBm
     */
    virtual bool computeExtCellInterference(MacNodeId eNbId, MacNodeId nodeId, inet::Coord coord, bool isCqi, double carrierFrequency, std::vector<double>* interference);

    /*
     * compute attenuation due to path loss and shadowing
     * @return attenuation expressed in dBm
     */
    double computeExtCellPathLoss(double threeDimDistance, double twoDimDistance, MacNodeId nodeId);
};

#endif /* NRCHANNELMODEL_H_ */
