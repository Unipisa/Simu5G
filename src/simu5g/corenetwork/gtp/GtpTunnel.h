//
//                  Simu5G
//
// Copyright (C) 2026 Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _GTP_TUNNEL_H_
#define _GTP_TUNNEL_H_

#include <ostream>

#include <inet/networklayer/common/L3Address.h>

#include "simu5g/common/LteTypes.h"

namespace simu5g {

/**
 * A fully qualified TEID (F-TEID, TS 29.244): the transport address of the
 * receiving end of a GTP-U tunnel, and the TEID that end allocated. The sending
 * end addresses its G-PDUs with it.
 */
struct FTeid
{
    inet::L3Address address;
    Teid teid = TEID_NONE;

    bool isSet() const { return teid != TEID_NONE; }
};

inline std::ostream& operator<<(std::ostream& os, const FTeid& fteid)
{
    return os << fteid.address.str() << "/teid=" << fteid.teid;
}

} //namespace

#endif
