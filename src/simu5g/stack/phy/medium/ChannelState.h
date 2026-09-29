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

#ifndef STACK_PHY_MEDIUM_CHANNELSTATE_H_
#define STACK_PHY_MEDIUM_CHANNELSTATE_H_

#include <map>
#include <ostream>
#include <queue>
#include <utility>
#include <vector>

#include <omnetpp.h>

#include <inet/common/geometry/common/Coord.h>

#include "simu5g/common/LteTypes.h"

namespace simu5g {

using namespace omnetpp;

/**
 * Identifies a radio link for the purpose of indexing its channel state.
 *
 * LOS/NLOS, shadowing and multipath fading are properties of a *link*, not of a
 * node: two links from the same transmitter to peers in different directions and
 * at different distances have independent realizations. Keying that state by node
 * is harmless for cellular, where a UE has exactly one link per channel-model
 * instance, but wrong for D2D, where one UE has many peers.
 *
 * The pair is stored normalized so that a link is the same key seen from either
 * end. Cellular callers construct the degenerate key {id, id}, one per UE.
 */
struct LinkKey
{
    MacNodeId a = NODEID_NONE;
    MacNodeId b = NODEID_NONE;

    LinkKey() = default;
    explicit LinkKey(MacNodeId node) : a(node), b(node) {}
    LinkKey(MacNodeId x, MacNodeId y)
        : a(num(x) <= num(y) ? x : y), b(num(x) <= num(y) ? y : x) {}

    bool operator<(const LinkKey& o) const
    {
        return num(a) != num(o.a) ? num(a) < num(o.a) : num(b) < num(o.b);
    }
    bool operator==(const LinkKey& o) const { return a == o.a && b == o.b; }
};

inline std::ostream& operator<<(std::ostream& os, const LinkKey& k)
{
    return num(k.a) == num(k.b) ? (os << k.a) : (os << "[" << k.a << "," << k.b << "]");
}

/**
 * The channel state a channel model evaluates links against, and updates as it
 * evaluates them: per link, whether it is in line of sight, its last shadowing
 * sample, its Jakes fading paths and the position its correlation distance is
 * measured from; per node, the recent positions its speed is derived from.
 *
 * The radio medium keeps one of these for every channel model that evaluates
 * links (CellularRadioMedium::getChannelState()). Each is that model's own: two
 * models evaluating the same link hold two independent entries for it.
 */
struct ChannelState
{
    /** A timestamped position: when it was recorded, and where. */
    typedef std::pair<simtime_t, inet::Coord> Position;

    /** The Jakes fading paths of one band: an angle of arrival and a delay spread per path. */
    struct JakesFadingData
    {
        std::vector<double> angleOfArrival;
        std::vector<simtime_t> delaySpread;
    };

    typedef std::vector<JakesFadingData> JakesFadingVector;          // one entry per band
    typedef std::map<LinkKey, JakesFadingVector> JakesFadingMap;
    typedef std::map<LinkKey, std::pair<simtime_t, double>> ShadowFadingMap; // when drawn, and the value in dB

    std::map<LinkKey, bool> losMap;                              // per link: whether it is in line of sight
    ShadowFadingMap shadowingMap;                                // per link: the last shadowing sample
    JakesFadingMap jakesFadingMap;                               // per link: the Jakes fading paths of every band
    JakesFadingMap jakesFadingMapBgUe;                           // the same, for the links of background UEs
    std::map<MacNodeId, std::queue<Position>> positionHistory;   // per node: its last two positions
    std::map<LinkKey, Position> lastCorrelationPoint;            // per link: the position the correlation distance is measured from
};

} // namespace simu5g

#endif
