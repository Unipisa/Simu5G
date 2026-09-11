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

#ifndef _LTE_ULBACKLOGREGISTRY_H_
#define _LTE_ULBACKLOGREGISTRY_H_

#include <functional>

#include "simu5g/common/LteCommon.h"

namespace simu5g {

class LteMacBuffer;

/**
 * The eNB/gNB MAC's registry of reported uplink backlog: what buffer status
 * reports have told it about each UE's queues. One mirror per key, where a key
 * is a pseudo-connection MacCid -- (ueId, BSR_UL_LCID_BASE + lcg) for the
 * per-LCG figures of an uplink report, (ueId, D2D_SHORT_BSR or
 * D2D_MULTI_SHORT_BSR) for the single figure of a D2D-typed report. The UL
 * scheduler treats the keys as its active connections and sizes grants from
 * the mirrors, draining them as it grants; the next report replaces whatever
 * a mirror holds.
 *
 * A mirror is an LteMacBuffer holding at most one PacketInfo (the reported
 * size and its timestamp), the same shape the grant loop drains for downlink
 * queues. The registry owns the mirrors.
 *
 * A plain class, deliberately not a module: it is the eNB MAC's bookkeeping,
 * created and consulted by it, and exercisable without a simulation. The
 * backlog callback decouples it from the scheduler it notifies.
 */
class UlBacklogRegistry
{
  public:
    typedef std::function<void(MacCid)> BacklogCallback;

  private:
    std::map<MacCid, LteMacBuffer *> mirrors_;
    BacklogCallback backlogCallback_;

  public:
    UlBacklogRegistry() {}
    ~UlBacklogRegistry();

    /// Called with the key whenever a report leaves its mirror with backlog.
    void setBacklogCallback(BacklogCallback callback) { backlogCallback_ = std::move(callback); }

    /**
     * Applies one reported figure to one key: a positive size replaces the
     * mirror's content and fires the backlog callback; zero empties the mirror
     * (the report said the queues are drained). Creates the mirror on first use.
     */
    void update(MacCid key, int64_t size, omnetpp::simtime_t timestamp);

    /// The mirror of a key; throws if no report has created it yet.
    LteMacBuffer *mirror(MacCid key) const;

    bool hasMirror(MacCid key) const { return mirrors_.count(key) != 0; }

    /// All keys reports have created so far (drained ones included).
    std::vector<MacCid> keys() const;

    /**
     * The node's per-LCG uplink mirrors, in logical channel group order. Their
     * occupancies together are the node's reported uplink backlog, which is what
     * an uplink grant is sized from: a grant is one transport block for the whole
     * UE. The D2D-typed mirrors carry a different direction's backlog and are
     * left out.
     */
    std::vector<LteMacBuffer *> ulMirrorsOf(MacNodeId nodeId) const;

    /// Empties the node's mirrors, keeping them registered (bearer teardown).
    void clearForNode(MacNodeId nodeId);

    /// Deletes the node's mirrors outright (the node leaves the cell).
    void eraseForNode(MacNodeId nodeId);

    /// Read-only view for inspection (WATCH).
    const std::map<MacCid, LteMacBuffer *>& mirrors() const { return mirrors_; }
};

} //namespace

#endif
