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

#ifndef NRCHANNELMODEL_3GPP38_901_H_
#define NRCHANNELMODEL_3GPP38_901_H_

#include "simu5g/stack/phy/channelmodel/NrChannelModel.h"

namespace simu5g {

/**
 * This channel model implements path loss, LOS probability, and shadowing according to
 * the following 3GPP specifications:
 * - 3GPP TR 38.901, "Study on channel model for frequencies from 0.5 to 100 GHz", v16.1.0, December 2019
 * - 3GPP TS 38.211, "NR; Physical channels and modulation", v16.2.0, July 2020
 * - 3GPP TS 38.214, "NR; Physical layer procedures for data", v16.2.0, July 2020
 *
 * The model supports 5G NR deployment scenarios including:
 * - Indoor Hotspot (InH) - 0.5-100 GHz
 * - Urban Microcell (UMi-Street Canyon) - 0.5-100 GHz
 * - Urban Macrocell (UMa) - 0.5-100 GHz
 * - Rural Macrocell (RMa) - 0.5-7 GHz
 *
 * Key features:
 * - Frequency-dependent path loss models compliant with 3GPP TR 38.901
 * - LOS/NLOS probability models for various scenarios
 * - Building penetration loss for indoor UEs (Section 7.4.3 of TR 38.901)
 * - Proper frequency handling: Hz for physical calculations, GHz for path loss formulas
 */
class NrChannelModel_3GPP38_901 : public NrChannelModel
{

  public:
    void initialize(int stage) override;

    /*
     * Compute LOS probability (taken from TR 38.901)
     *
     * @param d distance between UE and gNodeB
     * @param nodeId mac node id of UE
     */
    void computeLosProbability(double d, MacNodeId nodeId);

    /*
     * Compute the building penetration loss for indoor UE
     * See section 7.4.3 of TR 38.901
     *
     * @param threeDimDistance distance between UE and gNodeB (3D)
     */
    virtual double computePenetrationLoss(double threeDimDistance);

    /*
     * Compute the path-loss attenuation according to the selected scenario
     *
     * @param threeDimDistance distance between UE and gNodeB (3D)
     * @param twoDimDistance distance between UE and gNodeB (2D)
     * @param los line-of-sight flag
     */
    double computePathLoss(double threeDimDistance, double twoDimDistance, bool los) override;

    /*
     * UMa path loss model (taken from TR 38.901)
     *
     * @param threeDimDistance distance between UE and gNodeB
     * @param los line-of-sight flag
     */
    double computeUrbanMacro(double threeDimDistance, double twoDimDistance, bool los);

    /*
     * UMi path loss model (taken from TR 38.901)
     *
     * @param threeDimDistance distance between UE and gNodeB
     * @param los line-of-sight flag
     */
    double computeUrbanMicro(double threeDimDistance, double twoDimDistance, bool los);

    /*
     * UMa path loss model (taken from TR 38.901)
     *
     * @param threeDimDistance distance between UE and gNodeB
     * @param los line-of-sight flag
     */
    double computeRuralMacro(double threeDimDistance, double twoDimDistance, bool los);

    /*
     * InH path loss model (taken from TR 38.901)
     *
     * @param threeDimDistance distance between UE and gNodeB
     * @param los line-of-sight flag
     */
    double computeIndoor(double threeDimDistance, double twoDimDistance, bool los);

    /*
     * Compute std deviation of shadowing according to scenario and visibility
     *
     * @param sqrDistance distance between UE and gNodeB
     * @param nodeId mac node id of UE
     */
    double getStdDev(bool dist, MacNodeId nodeId);

    /*
     * Compute shadowing
     *
     * @param sqrDistance distance between UE and gNodeB
     * @param nodeId mac node id of UE
     * @param speed speed of UE
     */
    double computeShadowing(double sqrDistance, MacNodeId nodeId, double speed, bool cqiDl) override;

};

} //namespace

#endif /* NRCHANNELMODEL_3GPP38_901_H_ */
