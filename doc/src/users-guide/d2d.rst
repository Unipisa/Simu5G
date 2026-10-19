Device-to-device (D2D) communications
======================================

Simu5G includes a model of network-assisted device-to-device (D2D)
communications, in which two nearby UEs served by the same base station can
exchange data directly, over a UE-to-UE link, instead of relaying every packet
through the eNodeB/gNodeB. The base station retains control of the link: it
allocates the radio resources, drives the AMC, and decides (statically or
dynamically) whether each flow uses the direct D2D mode or the conventional
two-hop infrastructure mode. Both one-to-one and one-to-many (multicast) D2D
are supported, entirely below the IP layer, so applications are unaware of the
transport mode in use. D2D in Simu5G is a research prototype and is **not**
based on any specific 3GPP specification; the modeling is described in
A. Virdis, G. Nardini, G. Stea, "Modeling unicast device-to-device
communications with SimuLTE" (IWSLS2 2016) and in G. Nardini, A. Virdis,
G. Stea, "Modeling network-controlled device-to-device communications in
SimuLTE" (MDPI Sensors, 2018). Ready-to-run examples are provided under
``simulations/lte/d2d``, ``simulations/lte/d2d_multicast``,
``simulations/lte/d2d_multihop`` and ``simulations/nr/d2d``.

Enabling D2D
------------

D2D is turned on per node with a single switch. Setting ``**.hasD2D = true`` on
a UE and its serving base station selects the D2D-capable ``cellularNic``
variant on those nodes (an explicit ``cellularNic.typename`` still takes
precedence). The behavior of the direct links is then configured with a few
knobs:

- ``**.amcMode = "D2D"`` activates the D2D AMC pilot (the default ``"AUTO"`` is
  cellular-only).
- ``*.ue*.cellularNic.d2dInitialMode = true`` starts D2D-capable flows in
  direct mode rather than infrastructure mode.
- The CQI used for the direct links is either reported per link
  (``cellularNic.phy.enableD2DCqiReporting = true`` on the base station) or
  fixed (``**.usePreconfiguredTxParams = true`` together with ``**.d2dCqi``).
  One-to-many links always use a fixed CQI.
- Dynamic mode switching is driven from the RRC module of the D2D-capable base
  station NIC, which enables the periodic mode-selection tick
  (``rrc.hasD2DModeSelection = true``); the policy is selected with
  ``cellularNic.rrc.d2dModeSelection.typename`` (for example
  ``"D2DModeSelectionBestCqi"``, which keeps each flow on whichever mode
  currently has the better CQI).
- Frequency reuse among mutually non-interfering D2D pairs is available through
  the best-fit uplink allocator (``cellularNic.mac.schedulingDisciplineUl =
  "ALLOCATOR_BESTFIT"``, ``cellularNic.mac.reuseD2D = true`` and the associated
  conflict-graph parameters).

Module organization and extension points
-----------------------------------------

All D2D-specific modules live under the ``simu5g.stack.d2d`` package, kept
separate from the core LTE/NR stack: the D2D-capable NICs (``LteNicUeD2D``,
``LteNicEnbD2D``, ``NrNicUeD2D``, ``NrNicEnbD2D``) and, in sub-packages, the
D2D variants of the MAC, PHY, RLC, IP-to-NIC and RRC layers, the D2D AMC,
and the mode-selection modules. D2D links themselves are evaluated by the
radio medium's channel models, like every other link. The core stack reaches this
code only through a small set of interfaces, which are the natural extension
points for custom D2D behavior: the C++ interfaces ``ID2dMacUe``,
``ID2dMacEnb`` and ``ID2dAmc``, and the NED module
interface ``ID2DModeSelection``. To add a new mode-selection policy, subclass
``D2dModeSelectionBase`` (which implements the periodic evaluation and the
switch-notification machinery) and select it via
``cellularNic.rrc.d2dModeSelection.typename``.
