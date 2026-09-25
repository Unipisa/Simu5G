//
//                  Simu5G
//
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef STACK_PHY_MEDIUM_CELLULARRADIOMEDIUM_H_
#define STACK_PHY_MEDIUM_CELLULARRADIOMEDIUM_H_

#include <map>

#include <omnetpp.h>

#include "simu5g/common/LteCommon.h"

namespace simu5g {

using namespace omnetpp;

class IRadioEndpoint;

/**
 * The radio medium of a cellular network (see the NED documentation). So far
 * it keeps the registry of the radios on the medium: every PHY registers its
 * radio endpoint under its own MacNodeId, so a dual-stack UE's two PHYs are
 * two radios.
 */
class CellularRadioMedium : public cSimpleModule
{
  protected:
    std::map<MacNodeId, IRadioEndpoint *> radios;

  public:
    /** Adds a radio under the node id of its PHY. Throws if the id is taken. */
    virtual void addRadio(MacNodeId nodeId, IRadioEndpoint *radio);

    /** Removes the radio of the given node id. Throws if there is none. */
    virtual void removeRadio(MacNodeId nodeId);

    /** The radio of the given node id, or nullptr if none is registered. */
    virtual IRadioEndpoint *findRadio(MacNodeId nodeId) const;

    /** The radio of the given node id. Throws if none is registered. */
    virtual IRadioEndpoint *getRadio(MacNodeId nodeId) const;
};

} // namespace simu5g

#endif
