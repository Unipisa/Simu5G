//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _SIMU5G_CONNECTIONCONTROLUED2D_H_
#define _SIMU5G_CONNECTIONCONTROLUED2D_H_

#include "simu5g/stack/rrc/ConnectionControlUe.h"

namespace simu5g {

using namespace omnetpp;

class D2dBinder;

//
// D2D-capable variant of the ConnectionControlUe: reports the leg's D2D capability,
// and returns a torn-down sidelink bearer's identity to the sidelink pool. See
// ConnectionControlUeD2D.ned.
//
class ConnectionControlUeD2D : public ConnectionControlUe
{
  protected:
    // the network-wide sidelink state
    inet::ModuleRefByPar<D2dBinder> d2dBinder_;

  protected:
    void initialize(int stage) override;

  public:
    /// The leg is D2D-capable if its PHY is
    UeCapabilities getCapabilities() const override;

    /// A sidelink bearer's identity (the peer is a UE or a multicast group) returns to
    /// the network-wide sidelink pool of the pair, kept in the D2dBinder; an
    /// infrastructure bearer's is its base station's to release.
    void bearerReleased(DrbKey bearer) override;
};

} //namespace

#endif
