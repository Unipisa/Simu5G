//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

import inet.common.INETDefs;
import inet.common.TagBase;

namespace simu5g;

//
// @class LteRlcNewDataTag
// @brief Tag to indicate new data availability to MAC layer
//
// This tag replaces LteRlcPduNewData chunk and serves as a notification
// mechanism to inform the MAC layer that new data is available in the
// RLC transmission buffers. It carries no data fields - its mere presence
// on a packet indicates new data availability.
//
class LteRlcNewDataTag extends inet::TagBase
{
    // No fields needed - this is purely a notification tag
}
