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

#ifndef __SIMU5G_NTNPHYUE_H_
#define __SIMU5G_NTNPHYUE_H_

#include <map>

#include <inet/common/ModuleRefByPar.h>

#include "simu5g/mobility/georeference/GeographicReferenceSystem.h"
#include "simu5g/stack/phy/NrPhyUe.h"
#include "simu5g/stack/phy/NtnPropagationDelay.h"
#include "simu5g/stack/phy/antennamodel/IAntennaModel.h"

namespace simu5g {

class NtnPhyUe : public NrPhyUe
{

  protected:
    std::map<GHz, opp_component_ptr<LteChannelModel>> ntnChannelModel_;
    inet::ModuleRefByPar<LteChannelModel> primaryNtnChannelModel_;
    inet::ModuleRefByPar<IAntennaModel> ntnAntennaModel_;
    GeographicReferenceSystem *referenceSystem_ = nullptr;
    NtnPropagationDelay propagationDelay_;

    void initialize(int stage) override;
    void initializeChannelModels();
    void sendUnicast(LteAirFrame *airFrame) override;

    bool shouldSendViaTransparentNtn(MacNodeId destId) const;
    virtual bool sendUnicastViaNtn(LteAirFrame *airFrame);

  public:
    LteChannelModel *getReceptionChannelModel(const UserControlInfo *lteInfo) override;
    // Channel model this UE uses for the transparent NTN path on the given service-link
    // carrier, or nullptr if it has none.
    LteChannelModel *getNtnChannelModel(GHz carrierFreq) const;
    double computeReceivedBeaconPacketRssi(LteAirFrame *frame, UserControlInfo *lteInfo) override;
};

} // namespace simu5g

#endif
