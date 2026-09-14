Simu5G
======

5G NR and LTE/LTE-A user-plane simulation model, compatible with the
INET Framework.
Website: https://simu5g.org

Disclaimer
----------

  Simu5G is an open source simulator licensed under LGPL, and is based on
OMNeT++ framework which is available under the Academic Public License and
a commercial license (see https://omnest.com/licensingfaq.php). You are
solely responsible for obtaining the appropriate license for your use(s)
of OMNeT++. Intel is not responsible for obtaining any such licenses, nor
liable for any licensing fees due in connection with your use of OMNeT++.
Neither the University of Pisa, nor the authors of this software, are
responsible for obtaining any such licenses, nor liable for any licensing
fees due in connection with your use of OMNeT++.

  Simu5G is based on 3GPP specifications, which may involve patented and
proprietary technology. See https://www.3gpp.org/contact/3gpp-faqs#L5.
You are solely responsible for determining if your use of Simu5G requires
any additional licenses. Intel is not responsible for obtaining any such
licenses, nor liable for any licensing fees due in connection with your
use of Simu5G. Neither the University of Pisa, nor the authors of this
software, are responsible for obtaining any such licenses, nor liable
for any licensing fees due in connection with your use of Simu5G.

  This software is provided on an "as is" basis, without warranties of
any kind, either express or implied, including, but not limited to,
warranties of accuracy, adequacy, validity, reliability or compliance
for any specific purpose. Neither the University of Pisa, nor the
authors of this software, are liable for any loss, expense or damage
of any type that may arise in using this software.

  If you use this software or part of it for your research, please cite
our work:

  G. Nardini, D. Sabella, G. Stea, P. Thakkar, A. Virdis, "Simu5G – An
    OMNeT++ Library for End-to-End Performance Evaluation of 5G Networks,"
    in IEEE Access, vol. 8, pp. 181176-181191, 2020,
    doi: 10.1109/ACCESS.2020.3028550.

  If you include this software or part of it within your own software,
README and LICENSE files cannot be removed from it and must be included
in the root directory of your software package.

Contributors
------------

Simu5G was created at the University of Pisa, building on SimuLTE, the
LTE simulator developed by the same group. Core contributors:

- Giovanni Nardini (giovanni.nardini@unipi.it)
- Giovanni Stea (giovanni.stea@unipi.it)
- Antonio Virdis (antonio.virdis@unipi.it)

Since v1.3.1, Simu5G has been developed by Andras Varga and the OMNeT++ core
team. Notable contributions were also made by Mohamed Seliem (University
College Cork; initial SDAP code and QoS-aware scheduling) and Esteban Egea
Lopez (Universidad Politécnica de Cartagena; NR RLC). The WHATSNEW.md file
credits contributions release by release.

Development since v1.3
----------------------

Starting with v1.3.1, Simu5G has been going through a thorough overhaul.
The goals are to make the model follow the 3GPP architecture more closely,
to bring the code base in line with the practices of the INET Framework,
and thereby to build a sound foundation for new protocol features. The work
proceeds in small steps, each validated with regression tests; where a
change alters simulation results, the release notes say so and explain why.
The main milestones so far:

- **v1.4.0 - v1.4.2**: source tree and naming reorganized, more type-safe
  C++ code, packets made inspectable.

- **v1.4.3 - v1.4.5**: bearers established explicitly instead of being
  discovered from the traffic, protocol state carried in real header
  fields instead of packet tags, explicit initialization stages,
  incomplete MIMO code removed; bugs fixed in the PHY error model, the MAC
  and MEC.

- **v1.5.x**: Control Plane functionality collected into an RRC compound module
  (bearer management, registration, handover control); PDCP and RLC rebuilt
  as per-bearer entity modules; SDAP and QoS flows added.

- **v1.6.0**: standards-based RLC Unacknowledged and Acknowledged Mode for
  both NR (TS 38.322) and LTE (TS 36.322); radio link failure detection and
  RRC re-establishment.

- **v1.7.0**: central, spec-modeled bearer and QoS-flow configuration (the
  `BearerConfigurator` module); SDAP on by default in 5G standalone
  networks; NE-DC and SCG bearers; buffer status reporting and uplink
  scheduling per logical channel group; D2D factored out into a separate,
  optional package; the 3GPP propagation formulas audited, fixed and
  covered by unit tests.

Many of these releases require changes to existing ini files, or to code
that extends Simu5G. WHATSNEW.md describes the changes and the necessary
porting steps for each release.

Dependencies
------------

Simu5G requires OMNeT++ and the INET Framework. The versions each release
was tested with and is compatible with are listed in its entry in
WHATSNEW.md. The optional vehicular networking feature additionally
requires Veins and SUMO.

See INSTALL.md for installation instructions.

Simu5G Features
---------------

General

- eNodeB, gNodeB and UE models (LTE, NR, and dual-stack)
- Full LTE and NR user-plane protocol stack (SDAP, PDCP, RLC, MAC, PHY)
- Core network: EPC (PGW) and 5GC (UPF) models with GTP-U tunneling
- 5G standalone, EN-DC and NE-DC deployments

Bearers and QoS

- Data radio bearers configured centrally, either established up front or
  on demand when traffic first matches their definition
- EPC-style bearers selected by packet filters, and 5GC-style QoS flows
  mapped onto bearers
- QoS profiles per bearer, with the common standardized QCI/5QI
  characteristics predefined; used by QoS-aware scheduling
- QoS flow classification rules at the UPF (downlink) and the UE (uplink),
  reflective QoS

RRC

- RRC functionality is modeled, but RRC signaling between the UE and the base
  station is not simulated: procedures take effect through direct function
  calls between the modules involved, and where the duration of a procedure
  matters (handover, re-establishment), it is modeled with timers
- Bearer management: creation and release of the per-bearer PDCP and RLC
  entities at both ends of a bearer
- X2-based handover, dynamic cell association
- Radio link failure handling and RRC re-establishment

SDAP-PDCP

- QoS flow to DRB mapping, with per-bearer SDAP header configuration
- Per-bearer PDCP entities with header compression; NR PDCP reordering
- E-UTRA/NR Dual Connectivity: split bearers with configurable uplink
  split and leg selection, MCG and SCG bearers

RLC

- Transparent, Unacknowledged and Acknowledged Mode
- NR RLC per TS 38.322 and LTE RLC per TS 36.322 (segmentation,
  reassembly, ARQ, status reporting)

MAC

- HARQ functionalities
- Allocation management
- AMC
- Random access with preamble collisions
- Buffer status reporting per logical channel group, logical channel
  prioritization
- Scheduling Policies (MAX C/I and variants, Proportional Fair, DRR,
  QoS-aware Proportional Fair)
- Carrier Aggregation
- Support to multiple numerologies
- FDD, and TDD with a configurable downlink/uplink symbol split per carrier

PHY

- Channel Feedback management
- Stochastic channel model with path loss, LOS probability and shadowing
  per 3GPP TR 36.814, TR 36.873 or TR 38.901, and
  - inter-cell interference
  - fast fading
  - building penetration loss
  - (an)isotropic antennas
- Ideal channel model with configurable packet error rates, for protocol
  validation
- Background cells and background UEs, for large-scale scenarios

Advanced features

- X2 communication support
- CoMP Coordinated Scheduling support
- Device-to-device communications (optional feature; one-to-one, multicast
  and multihop)
- Support for vehicular mobility (optional feature; integration with Veins)
- ETSI-compliant model of Multi-access Edge Computing (MEC) systems

Applications

- Voice-over-IP (VoIP)
- Constant Bit Rate (CBR)
- Trace-based Video-on-demand (VoD)
- Burst traffic, alert messaging, D2D multihop dissemination
- MEC applications (e.g. real-time video streaming, warning alert,
  request-response, radio network information service test)

Real-time emulation support
---------------------------

Simu5G supports real-time network emulation capabilities. Navigate to
one of the examples included in the "emulation" folder and take a look
at the README file included therein.

Documentation
-------------

- The website (https://simu5g.org) hosts the user's guide, the tutorials
  and showcases, and the NED reference documentation. Its sources are
  in `doc/src`.
- Example simulations are in `simulations/` (grouped into `lte/` and
  `nr/`), tutorials in `tutorials/`, showcases in `showcases/`, emulation
  examples in `emulation/`.
- WHATSNEW.md describes the changes in each release.

Testing
-------

Changes to Simu5G are validated with three kinds of tests:

- **Fingerprint tests** (`tests/fingerprint`) check that the example
  simulations follow the same event trajectory as recorded, so changes
  meant to preserve behavior can be verified to do so.
- **Statistical tests** compare the scalar results of the example
  simulations against baselines kept in the Simu5G-statistics repository
  (https://github.com/inet-framework/Simu5G-statistics), so the effect of
  changes that alter behavior can be evaluated.
- **Unit tests** (`tests/unit`) exercise classes directly, without
  running a simulation; for example, the 3GPP propagation formulas are
  graded against oracle values computed from the reports.

See INSTALL.md for how to run the fingerprint and unit tests.

Limitations
-----------

- User Plane only: Control Plane signaling (RRC, NAS, session management)
  is not modeled. Its effects are modeled directly: bearers come from
  configuration, and procedures such as handover and RRC re-establishment
  are modeled by their timers.
- No MIMO support
- Device-to-device communication is a research prototype, not based on
  the 3GPP sidelink specifications
- IPv4 user plane only
- No scheduling request: a UE without an uplink grant obtains one through
  the random access procedure
- No prioritized bit rate in logical channel prioritization
