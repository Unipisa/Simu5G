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

#include "simu5g/stack/phy/medium/CellularRadioMedium.h"

namespace simu5g {

Define_Module(CellularRadioMedium);

void CellularRadioMedium::addRadio(MacNodeId nodeId, IRadioEndpoint *radio)
{
    Enter_Method("addRadio");
    if (!radios.emplace(nodeId, radio).second)
        throw cRuntimeError("CellularRadioMedium::addRadio(): a radio is already registered for node %d", (int)num(nodeId));
}

void CellularRadioMedium::removeRadio(MacNodeId nodeId)
{
    Enter_Method("removeRadio");
    if (radios.erase(nodeId) == 0)
        throw cRuntimeError("CellularRadioMedium::removeRadio(): no radio is registered for node %d", (int)num(nodeId));
}

IRadioEndpoint *CellularRadioMedium::findRadio(MacNodeId nodeId) const
{
    auto it = radios.find(nodeId);
    return it == radios.end() ? nullptr : it->second;
}

} // namespace simu5g
