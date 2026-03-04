# What's New in Simu5G

## v1.4.3 (2026-02-18)

This release represents a major milestone in the complete overhaul of the Simu5G
codebase to make it architecturally more compliant with the 3GPP
specifications, modernize the code, and adopt the best practices of the INET
Framework on which it is based. The goal is to pave the way for a clean
implementation of new protocol features such as TSN support.

Tested with INET-4.5.4 and OMNeT++ 6.3, updated for INET-4.6.0 compatibility.

Key achievements in this release:

- **More explicit Control Plane modeling**: Simu5G is advertised as a User Plane
  simulator, but since it was also used to model dynamic scenarios such as
  handovers, it always contained elements of the Control Plane distributed across
  various modules. The new direction is to make these elements more explicit and
  centralized, such as creating dedicated RRC (Radio Resource Control) and
  Session Management Function (SMF) implementations. It is an explicit non-goal
  to simulate Control Plane messaging -- its functionality will be implemented
  with C++ method calls across modules. Thus, Simu5G remains a User Plane
  simulator, but with the possibility to more faithfully model dynamic
  scenarios with heavy Control Plane involvement. While this goal is not fully
  realized in this release, many changes point into that direction.

- **Control info refactoring**: Cleaned up `UserControlInfo` and
  `FlowControlInfo` by removing 5+ unused fields and splitting out smaller,
  focused tags. For example, IPv4 addresses, only used between `Ip2Nic` and PDCP,
  have been factored out into an `IpFlowInd` tag. This improves modularity,
  reduces coupling between protocol layers, and makes the code easier to
  maintain and extend.

- **Added vital missing fields to PDCP and MAC headers**: Protocol layers now
  use proper header fields instead of "tunnelling" information via
  `UserControlInfo` and `FlowControlInfo` packet tags that would not exist in a
  real implementation. For example, PDCP sequence numbers are now carried in
  PDCP headers, and LCIDs are stored in MAC PDU subheaders. This makes the
  simulation more realistic and packet contents more inspectable in Qtenv.

- **Explicit setup of logical connections instead of on-the-fly discovery**:
  This is a key architectural change, which also largely motivated the
  previous items. In previous iterations of Simu5G, data structures associated
  with logical connections / bearers were created in each protocol layer as they
  encountered packets that belonged to new connections. Moreover, part of the
  connection state was carried along by the packets in `FlowControlInfo` tags
  instead of stored inside the protocol. While this modeling approach still
  allowed for faithful simulation of the traffic while keeping the
  implementation simple, it has become a roadblock for implementing complex
  dynamic scenarios where connections come and go. In this iteration,
  centralized session and bearer management (SMF-like functionality) was added
  to the `Binder` module, and RRC modules were added to NICs to carry out local
  configuration. This brings the architecture closer to the 3GPP control/user
  plane separation, making it easier to implement features like handovers
  correctly. This is work in progress: SMF is still part of `Binder` and not a
  separate module, and connection setup is still triggered by the first packet
  of the connection hitting PDCP on the way out. However, moving the SMF code
  into its own module will be trivial, and the single `Binder` method call in
  PDCP can now be easily replaced with static configuration or with calls from a
  more detailed Control Plane implementation.

- **Removed incomplete MIMO support**: Removed MIMO-related code and parameters.
  The existing MIMO code was incomplete (e.g., PMI values were computed but
  never used). Removing it simplifies the codebase and model parameterization,
  and avoids confusion about capabilities. MIMO support will be added in a future
  release, with a different approach.

- **Initialization cleanup**: Reorganized module initialization into well-defined,
  Simu5G-specific init stages. This eliminates hidden cross-module dependencies,
  makes the initialization order explicit and verifiable, and prevents subtle bugs
  caused by modules accessing uninitialized data in other modules.

Further notable changes:

- In UE models, `masterId` and `nrMasterId` were renamed to `servingNodeId` and
  `nrServingNodeId`. The old names were confusing because "Master" has a
  specific meaning in Dual Connectivity (Master eNB vs Secondary gNB), unrelated
  to the UE's serving node.

- In UE models, the `macCellId`, `nrMacCellId` parameters were removed. In
  practice, the code already used the serving node ID as cell ID.

- `macNodeId` assignment was moved to NED, and now it is based on the new
  `simu5g_seq()` NED function that generates an integer sequence. This replaces
  the earlier approach where node IDs were assigned by `Ip2Nic` during
  initialization, and stored back into the module parameters for other modules to
  use.

- `LteRlcPduNewData` and `LteRlcSdu` packet chunks were converted to packet
  tags, as they represent internal metadata rather than actual protocol data.

- In the C++ code, merged the `ENODEB` and `GNODEB` node type enum values into a
  single `NODEB` value, with a separate `isNr` flag where needed. This change
  simplified a large number of "if" conditions throughout the codebase.

To port your existing Simu5G simulations to this version, apply the following
changes to the ini files:

- Change `masterId` to `servingNodeId` (and `nrMasterId` to `nrServingNodeId`),
  unless it refers to the Master/Secondary distinction in a Dual Connectivity
  setup.

- Remove `macCellId` and `nrMacCellId` parameter assignments for UE modules.

- Delete ini entries that set the following removed MIMO-related parameters:
  `numRus`, `ruRange`, `ruStartingAngle`, `ruTxPower`, `antennaCws`,
  `muMimo`, `pmiWeight`, `lambdaMinTh`, `lambdaMaxTh`, `lambdaRatioTh`,
  `feedbackGeneratorType`.

- For `initialTxMode`, the following values are no longer valid:
  `SINGLE_ANTENNA_PORT5`, `OL_SPATIAL_MULTIPLEXING`, `CL_SPATIAL_MULTIPLEXING`,
  `MULTI_USER`. Remove the parameter assigment to use the default.

There are many more changes that potentially affect existing simulations, and
projects extending, or built on top of, Simu5G. They cannot all be covered here
in detail - see the git history for details.


## v1.4.2 (2025-11-27)

This is primarily a bugfix release.

- Pdcp: Fixed Dual Connectivity bug where separate PDCP entities were
  incorrectly created for LTE and NR legs of a Split Bearer instead of using a
  single shared entity. This fix breaks RLC-UM packet loss statistics which
  (incorrectly) inferred packet loss from PDCP sequence numbers.

- RlcUm: Removed packet loss statistics that incorrectly relied on PDCP sequence
  numbers (PDCP sequences are not contiguous in Dual Connectivity setups)

- PacketFlowManager: Renamed to PacketFlowObserver, updated NED documentation.

- Statistics collection refined, e.g. remove recording "sum" and/or "mean" where
  it does not make sense; use new "rateavg" filter for computing average
  throughput.

- Binder: New utility functions: isGNodeB(), getUeNodeId().

- Apps: Added sequence numbers to VoIP and VoD packet names, to facilitate
  tracing with Qtenv.

- NED documentation: Added content to simu5g-index.ned including version number
  and WHATSNEW.


## v1.4.1-sdap-2 (2026-03-03)

This release improves on the simu5g-1.4.1-sdap release that added SDAP (Service
Data Adaptation Protocol) layer support to Simu5G. Contributed by Andras Varga
(OMNeT++ Core Team).

The most important changes:

- New simulations that exercise the code more: Multi-UE, multi-app, multi-QFI
  configurations were added into `omnetpp_drb.ini` under `nr/standalone`. Based
  on the Simu5G#294 bug report by Jonathan "Toaaster" Ebert.

- Fixed QFI propagation: QFI was originally added to packets by the application
  (`VoipSender`) as a packet tag (`QfiTag`). However, this tag did not make it to
  UPF, because it was already stripped by the local PPP interface on
  transmission. This mechanism was replaced by the VoipSender app setting DSCP
  on the packet, which UPF now interprets as QFI (simplified PDR matching). From
  then on, QFI is now carried through the GTP-U tunnel in the GTP header
  (mirroring the real 5G PDU Session Container extension header), instead of
  relying on QoS tags that were being stripped by PPP. The gNB extracts QFI from
  the GTP-U header to restore QoS tags for SDAP routing.

- DRB configuration changes: The DRB configuration is now split between SDAP and
  MAC layers, each only knowing as much as they need for their operation.
  QFI-to-DRB routing configuration went into `sdap.drbConfig`, while QoS
  parameters for the scheduler (GBR, delay budget, PER, priority) went into
  `mac.drbQosConfig`. Moreover, DRB configuration is now specified in JSON,
  replacing the previous text file-based configuration.

- Fixed multi-UE DL starvation (fixes Simu5G#294): `MacDrbMultiplexer`
  incorrectly used LCID as the `nrRlc[]` array index, assuming LCID equals the
  DRB index. When multiple UEs shared the same DRB, only the first UE received
  data. Fixed by learning the LCID-to-gate mapping from RLC-to-MAC traffic.

- MEC fixes: There were several bug fixes in the MEC code, such as
  `MecOrchestrator` (contextId counter was never incremented), `MecOrchestrator`
  (missing return after failure path causing end-iterator dereference),
  `MecAppBase` (eliminate undisposed objects), fix uninitialized variables in
  various modules (fixing long-standing fingerprint failures of certain MEC
  simulations in debug mode).


## v1.4.1-sdap (2025-10-06)

Compatible with OMNeT++ 6.2.0 and INET 4.5.4.

This specialized branch release introduces SDAP protocol support, multiple DRBs
and advanced QoS capabilities to Simu5G for enhanced 5G network simulations.
Please note that future main releases may not include these features or may
incorporate them in a different form, as the primary development focus remains
on architectural refactoring and foundational improvements. The changes were
contributed by Mohamed Seliem (University College Cork), with improvements by
Andras Varga (OMNeT++ Core Team).

Reference paper: "QoS-Aware Proportional Fairness Scheduling for Multi-Flow 5G
UEs: A Smart Factory Perspective". Mohamed Seliem, Utz Roedig, Cormac Sreenan,
Dirk Pesch. IEEE MSWiM, 2025.

New Features:

- Added an SDAP protocol implementation with reflexive QoS capabilities (NrSdap
  and ReflectiveQosTable modules). Available using the NRUeSdap (UE) and
  gNodeBSdap (gNodeB) node types that contain the NRNicUeSdap and NRNicEnbSdap
  NIC types, respectively.

- DRB (Data Radio Bearer) support with multi-QFI/QoS handling for realistic 5G
  bearer management simulations. This feature is available using NRUeDrb (UE)
  and gNodeBDrb (gNodeB) node types that contain the NRNicUeDrb and NRNicEnbDrb
  NIC types, respectively. It can be configured using the numDrbs parameter.
  QFI-to-DRB mappings can be defined in a context file (see SDAP's
  qfiContextFile parameter) with 5QI parameters and QoS requirements.

- QoSAwareScheduler with QFI-based Proportional Fair scheduling using QfiContextManager.
  Enable QoS scheduling with LteMacEnb.schedulingDisciplineDl/Ul="QOS_PF".

- Better representation of compressed headers in PDCP. (Note that header compression is
  disabled by default; enable using PDCP's headerCompressedSize parameter.)

- New example simulations: simulations/nr/standalone/omnetpp_sdap.ini and omnetpp_drb.ini,
  each with Standalone, VoIP-DL, and VoIP-UL configurations demonstrating SDAP functionality
  and multi-DRB support with QoS-aware scheduling.


## v1.4.1 (2025-10-06)

This is a minor update that brings further refactoring of the C++ code for clarity,
improvements in the C++ interface of the Binder module, and some minor bug fixes.
These improvements were contributed by Andras Varga (OMNeT++ Core Team).

Notable changes:

- Binder: Partial rationalization of the C++ interface, via
  renaming/replacing/removing methods. See the git history for changes.

- Updated IP addresses in the IPv4 configuration files: use 10.x.x.x
  addresses for the Core Network, and 192.168.x.x addresses for external
  addresses

- Visual improvement: node IDs are now displayed over module icons

- In MEC, do not use module IDs for bgAppId, deviceAppId and other IDs, and do
  not encode module ID into module names. That practice made simulations brittle
  for regression testing via fingerprints.

- PDCP: Eliminated tweaking of srcId/destId in FlowControlInfo when sending
  downlink packets over the X2 link in a Dual-Connectivity setup.

- Various additional fixes and changes to improve code quality.


## v1.4.0 (2025-09-18)

Compatible with **OMNeT++ 6.2.0** and **INET 4.5.4**.

This release marks an important milestone in the ongoing transformation of
Simu5G. While not introducing behavioral changes, this intermediate release
focuses on restructuring the codebase to improve clarity, safety, and
maintainability. Major updates include a reorganized directory structure,
enforcing a consistent naming convention, making make packets more easily
inspectable, and refactoring of parts of the C++ code to pave the way for
changes in future versions. Although the release is not source-compatible with
previous versions, existing simulations will continue to work unchanged once
adjusted to follow the various rename operations. These improvements were
contributed by Andras Varga (OMNeT++ Core Team).

Renames:

- Sources are now under src/simu5g/ instead of just src/, so that C++ includes
  start with "simu5g/". This helps identifying Simu5G includes when Simu5G is
  used as a dependency of other projects.

- Some folders were moved inside the source tree to a more logical location. For
  example, the simu5g/nodes/mec/ subtree was promoted to simu5g/mec/.

- Several source folders were renamed to more closely follow the all-lowercase
  convention. For example, mec/UALCMP/ became mec/ualcmp/, and mec/MECPlatform
  became just mec/platform.

- Several classes were renamed to ensure that only the first letters of acronyms
  are uppercase. For example, MECHost became MecHost.

- Several parameters were renamed to enforce camelcase names. For example,
  bs_noise_figure became bsNoiseFigure, and fading_paths became numFadingPaths.
  If you have existing Simu5G simulations, review the ini files carefully and
  update the parameter assignments accordingly. (Caveat: Assignment lines that
  refer to the old names will be simply ignored by the simulation -- there is no
  error message for that!)

- The PdcpRrc modules were renamed to just Pdcp. Likewise, pdcpRrc submodules
  in NIC compound modules became pdcp.

Further refactoring:

- Several protocol header classes, while defined in msg files, contained heavy
  customization in C++ code, including the addition of new fields. Since the
  writing of those classes, the message compiler in OMNeT++ gained enough
  features so that most of the customizations were no longer needed, and the
  desired effect could be achieved in msg files only. This refactoring has the
  benefit of making packets more inspectable from Qtenv, and packet contents can
  now be serialized using parsimPack (useful for more thorough fingerprint
  tests).

- MacCid is a central data type that pairs an LCID with a nodeId to uniquely
  identify a logical channel. It used to be a packed integer, and now it was
  turned into a C++ class with separate fields for the node ID and LCID and with
  accessor methods, for increased type safety.

- carrierFrequency used to be a variable of the type double throughout the
  codebase. The type was changed to GHz (using INET's units.h) for increased
  type safety. This also helped identifying a bug in certain channel models
  (LteRealisticChannelModel, BackgroundCellChannelModel) where a double
  representing GHz instead of Hz was used in computing path loss, resulting
  in underestimated path loss values.

- Binder received several WATCHes for increased transparency in Qtenv, and
  an overhaul of a subset of its API and internal data structures.

- In the Pdcp modules, the unused EUTRAN_RRC_Sap port (and associated handling
  code) was removed.

- Refactoring of internals in several protocol modules, including MAC, PDCP and
  RLC implementations.

Build:

- Made the command line build consistent with the IDE build. src/Makefile is now
  generated/updated implicitly on every build, no need to type "make makefiles".

## v1.3.1 (2025-09-18)

This is a minor update for Simu5G-1.3.0. In addition to fixing regressions
in the previous release and making some cosmetic improvements, the main highlight
of this release is the revamp of the fingerprint test suite, which now provides
a more comprehensive safety net against future regressions. Changes in this
release were contributed by Andras Varga (OMNeT++ Core Team).

Changes:
- Example simulations: Marked abstract configs as such (abstract=true) in omnetpp.ini files
- FlowControlInfo's MacNodeId fields are now properly shown in Qtenv object inspectors
- Replaced EV_ERROR << lines with throwing cRuntimeError
- TrafficLightController: fixed startState NED parameter (also changed type from int to string)
- Fingerprints: CSV files merged into simulations.csv, added missing simulations,
  standardized on the set of fingerprints computed (tplx, ~tNl, sz), translated
  gen_runallexamples.py into Python and improved it

Fix regressions in v1.3.0:
- MECResponseApp: fixed wrong @class annotation
- BackgroundScheduler: fix "binder_ not initialized" error
- MecRequestForegroundApp, MecRequestBackgroundGeneratorApp: add back lost parameter defaults
- BackgroundCellTrafficManager: fix "Not implemented" thrown from getBackloggedUeBytesPerBlock()
- tutorials/nr/omnetpp.ini: fix missing unit for txPower parameter (dBm)

## v1.3.0 (2025-02-06)

- Compatible with **OMNeT++ 6.1.0** and **INET 4.5.4**
- New modules: MultiUEMECApp, MecRnisTestApp, UeRnisTestApp
- Added NED documentation for modules
- Increased reusability of modules via changes such as replacing hardcoded module
  paths in the C++ code with NED parameters (binderModule, macModule, etc.), and
  elimination of ancestorPar() calls by introducing local parameters instead. *
- Other NED adjustments, such as removal of unused NED parameters and splitting
  NED files to have one module per file. See doc/NED-changes.txt for details. *
- Extensive C++ modernization, and adaption of more OMNeT++ best practices. *
- Various bug fixes.
- Changes marked with an asterisk were contributed by Andras Varga (OMNeT++ Core
  Team).

## v1.2.3 (2025-01-10)

- Added support for **OMNeT++ 6.1.0** and **INET 4.5.4**.

## v1.2.2 (2023-04-19)

- Compatible with **OMNeT++ 6.0.1** and **INET 4.5**.
- Tested on Ubuntu 22.04 and macOS Ventura.

## v1.2.1 (2022-07-19)

- Compatible with **OMNeT++ 6.0** and **INET 4.4.0**.
- Tested on Ubuntu 20.04.
- Modifications to support the latest versions of OMNeT++ 6.0 and INET v4.4.0.
- Refactoring of simulation and emulation folders.
- Various bug fixes.

## v1.2.0 (2021-08-30)

- Compatible with **OMNeT++ 6.0 (pre10 and pre11)** and **INET 4.3.2**.
- Tested on Ubuntu 16.04, 18.04, 20.04, macOS Catalina, and Windows 7.
- Added modelling of **ETSI MEC** entities.
- Support for **real-time emulation capabilities** (Linux OS only).
- Modelling of background cells and background UEs for **larger scale** simulations and emulations.
- Several bug fixes.

## v1.1.0 (2021-04-16)

- Compatible with **OMNeT++ 5.6.2** and **INET 4.2.2**.
- Tested on Ubuntu 16.04, 18.04, 20.04, macOS Catalina, and Windows 7.
