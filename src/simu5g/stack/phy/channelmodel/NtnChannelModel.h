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

#include "simu5g/stack/phy/channelmodel/LteRealisticChannelModel.h"

namespace simu5g {

class NtnChannelModel : public LteRealisticChannelModel
{
  protected:
    inet::Coord lastUeCoord_;
    inet::Coord lastNtnCoord_;

  public:
    double getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord coord, bool cqiDl) override;
    void computeLosProbability(double d, MacNodeId nodeId);
};

} // namespace simu5g

#endif /* NTNCHANNELMODEL_H_ */
