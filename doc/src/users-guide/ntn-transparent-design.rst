Transparent NTN integration plan (satellite + NTN gateway)
===============================================================

Goal
----

Add a first NTN-capable control/data model to Simu5G where:

* UE association remains unchanged (UE <-> gNB/eNB).
* A terrestrial gNB can optionally be associated with:

  * one transparent satellite node;
  * one NTN gateway node (ground).

* The satellite path is represented as metadata and topology relations,
  without changing terrestrial scheduling logic in the first step.


Design principles
-----------------

1. **Do not break existing terrestrial behavior**
   Keep all current UE/gNB procedures untouched when no NTN metadata is configured.
2. **Model NTN as optional overlays over existing gNB metadata**
   Reuse gNB identity and serving-cell concepts; add association metadata on top.
3. **Keep transparent-satellite semantics explicit**
   In transparent mode, radio/control termination remains on ground entities (gNB + NTN gateway).
4. **Use Binder as the source of truth for global relations**
   Binder already holds cross-module registries, so NTN relation maps should live there too.


Data structures to add
----------------------

1) New NTN info records (parallel to ``EnbInfo``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Add two structs in ``LteCommon.h`` near existing node info structs:

* ``SatelliteInfo``

  * ``bool init``
  * ``MacNodeId id``
  * ``opp_component_ptr<cModule> satelliteModule``
  * optional orbital/dynamic metadata for future use (altitude, inclination, ephemeris source id)

* ``NtnGatewayInfo``

  * ``bool init``
  * ``MacNodeId id``
  * ``opp_component_ptr<cModule> gatewayModule``
  * optional transport metadata (gateway IP/backhaul delay budget)

2) gNB-to-NTN association record
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Add a dedicated relation struct:

* ``GnbNtnAssociation``

  * ``MacNodeId gnbId``
  * ``MacNodeId ntnGatewayId`` (``NODEID_NONE`` if absent)
  * ``MacNodeId satelliteId`` (``NODEID_NONE`` if absent)
  * ``bool isTransparent`` (true for this phase)

This keeps NTN topology orthogonal to existing ``EnbInfo`` and avoids
expanding terrestrial structs with many optional fields.

3) Binder storage
^^^^^^^^^^^^^^^^^

In ``Binder``:

* Add vectors/maps like:

  * ``std::vector<SatelliteInfo*> satelliteList_``
  * ``std::vector<NtnGatewayInfo*> ntnGatewayList_``
  * ``std::map<MacNodeId, GnbNtnAssociation> gnbNtnAssoc_``

* Optional reverse indexes for fast lookups:

  * ``std::map<MacNodeId, std::set<MacNodeId>> satelliteToGnbs_``
  * ``std::map<MacNodeId, std::set<MacNodeId>> ntnGatewayToGnbs_``


Node typing and registration
----------------------------

1. Extend ``RanNodeType`` with NTN-specific node kinds, e.g.:

* ``SATELLITE_NODE``
* ``NTN_GATEWAY_NODE``

2. Update helper conversion methods:

* ``nodeTypeToA``
* ``aToNodeType``
* any node-type helpers used for validation/logging

3. Binder registration APIs:

* ``registerSatelliteNode(MacNodeId satId, cModule* satModule)``
* ``registerNtnGatewayNode(MacNodeId ntnGwId, cModule* gwModule)``
* ``setGnbNtnAssociation(MacNodeId gnbId, MacNodeId ntnGwId, MacNodeId satId, bool transparent=true)``
* ``const GnbNtnAssociation* getGnbNtnAssociation(MacNodeId gnbId) const``



Node ID bounds plan (LteCommon.msg)
-----------------------------------

``LteCommon.msg`` already defines the node-id ranges, so NTN support should reserve
explicit non-overlapping ranges there (instead of leaving IDs implicit):

* Keep ``NODEID_NONE = 0``.
* Keep ``ENB_MIN_ID = 1`` and ``ENB_MAX_ID = 1023`` for terrestrial eNB/gNB nodes.
* Add ``NTN_GW_MIN_ID``/``NTN_GW_MAX_ID`` and ``SAT_MIN_ID``/``SAT_MAX_ID`` directly
  after the eNB/gNB range.
* Move UE/BG UE lower bounds upward so UE ids start strictly above the new NTN ranges.
* Keep ``UE_MAX_ID < MULTICAST_DEST_MIN_ID`` unchanged to preserve multicast separation.

Suggested concrete phase-1 split (fits current ``unsigned short`` IDs):

* ``NTN_GW_MIN_ID = 1024``
* ``NTN_GW_MAX_ID = 1279``
* ``SAT_MIN_ID = 1280``
* ``SAT_MAX_ID = 1535``
* ``UE_MIN_ID = 1536`` (then recompute ``NR_UE_MIN_ID`` and ``BGUE_MIN_ID`` accordingly)

Validation rules to implement with these bounds:

* ``NtnGatewayInfo.id`` must be in ``[NTN_GW_MIN_ID, NTN_GW_MAX_ID]``.
* ``SatelliteInfo.id`` must be in ``[SAT_MIN_ID, SAT_MAX_ID]``.
* ``EnbInfo``/gNB ids remain constrained to ``[ENB_MIN_ID, ENB_MAX_ID]``.
* UE ids stay in the UE range only.

How NTN metadata integrates with terrestrial gNB metadata
---------------------------------------------------------

Recommended approach for this phase:

* Keep ``EnbInfo`` unchanged for compatibility.
* Keep UE serving-node mapping unchanged (UE still points to the serving gNB/eNB).
* Add NTN relation lookup as an **optional second lookup** from serving gNB id:

  1. UE -> serving gNB via existing ``servingNode_``;
  2. gNB -> NTN tuple via ``gnbNtnAssoc_`` (if present).

This preserves all existing flows and only introduces NTN-aware behavior
where explicitly enabled.


Transparent satellite association model
---------------------------------------

For transparent NTN, the association path should be represented as:

``UE -> gNB (ground) -> ntnGateway (ground) -> satellite (transparent relay)``

Association constraints to validate at configuration/register time:

* gNB must exist before setting NTN relation.
* NTN gateway and satellite must exist (unless explicitly optional in scenario bootstrap).
* one gNB has at most one active transparent NTN tuple in phase 1.
* a satellite or NTN gateway may serve multiple gNBs.


Suggested initialization flow
-----------------------------

1. Create/register terrestrial nodes as today (including gNB metadata).
2. Register satellite and NTN gateway nodes in Binder.
3. Apply NTN associations for selected gNBs.
4. During runtime, modules that need NTN context query Binder using serving gNB id.


First implementation milestone (minimal, low-risk)
---------------------------------------------------

* Add data structures and Binder APIs only.
* Add simple NED parameters on gNB (optional ids or module paths):

  * ``ntnGatewayId`` (default ``NODEID_NONE``)
  * ``satelliteId`` (default ``NODEID_NONE``)
  * ``isTransparentNtn`` (default ``false``)

* During gNB init, if NTN params are set, call ``setGnbNtnAssociation``.
* No changes to UE attach/handover logic.
* Add logs/stat counters that expose NTN association presence.


Second milestone (behavioral integration)
-----------------------------------------

* Inject satellite+gateway-dependent propagation/backhaul delay into
  selected paths while keeping attach semantics unchanged.
* Add handover/reselection policies that can account for NTN overlay data.


Why this decomposition works
----------------------------

* It mirrors existing Simu5G architecture: Binder as global registry,
  node-local modules as runtime actors.
* It avoids risky refactors of mature terrestrial data paths.
* It gives a clean path from metadata-only support to full NTN behavior.
