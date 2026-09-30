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
 * evaluates them: per node, the recent positions its speed is derived from.
 *
 * The radio medium keeps one of these for every channel model that evaluates
 * links (CellularRadioMedium::getChannelState()). Each is that model's own: two
 * models evaluating the same link hold two independent entries for it. Whether
 * a link is in line of sight, its shadowing and its Jakes fading paths are not
 * part of it: the medium keeps those once per link
 * (CellularRadioMedium::getLosMap(), getShadowingMap(), getJakesFadingMap()).
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

    /**
     * Whether a link is in line of sight, and where the link's two radios were,
     * in the order of its key, when that was decided.
     */
    struct LosSample
    {
        inet::Coord positionA; // where LinkKey::a was
        inet::Coord positionB; // where LinkKey::b was
        bool los = false;
    };
    typedef std::map<LinkKey, LosSample> LosMap;

    typedef std::vector<JakesFadingData> JakesFadingVector;          // one entry per band
    typedef std::map<LinkKey, JakesFadingVector> JakesFadingMap;
    /**
     * A link's last shadowing sample: when it was drawn, where the link's two
     * radios were then, in the order of its key, and its value in dB.
     */
    struct ShadowingSample
    {
        simtime_t time;
        inet::Coord positionA; // where LinkKey::a was
        inet::Coord positionB; // where LinkKey::b was
        double value = 0;
    };
    typedef std::map<LinkKey, ShadowingSample> ShadowFadingMap;

    std::map<MacNodeId, std::queue<Position>> positionHistory;   // per node: its last two positions
};

} // namespace simu5g

#endif
