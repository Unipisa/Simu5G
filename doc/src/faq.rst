:hide-footer:

Frequently Asked Questions
==========================


.. question:: Where can I download the Simu5G code?

   Welcome on board! Visit :doc:`this page <install>` to
   download Simu5G.

.. question:: Which OMNeT++ and INET versions do I need?

   It depends on the Simu5G version. Each release's entry in the `release
   notes <https://github.com/Unipisa/Simu5G/blob/master/WHATSNEW.md>`__ lists
   the OMNeT++ and INET versions it was tested with and is compatible with.
   If you install Simu5G with ``opp_env``, the matching versions are installed
   automatically. See the :doc:`install` page.

.. question:: My simulation, written for an older Simu5G version, no longer
   works. What should I do?

   Since v1.3.1, Simu5G has been going through a thorough overhaul, and many
   releases renamed or removed modules and parameters. The `release notes
   <https://github.com/Unipisa/Simu5G/blob/master/WHATSNEW.md>`__ describe the
   changes and the necessary porting steps for each release; go through the
   entries of all releases between your old version and the new one. Note
   that ini file lines that assign a renamed or removed parameter are
   silently ignored, so check your configuration even if the simulation
   runs.

.. question:: How is Simu5G different from the Vienna 5G SL simulator?

   The Vienna 5G SL simulator is a MATLAB-based simulator that
   allows one to evaluate average PHY-layer performance by means of
   Monte Carlo simulations. A system-level version of it, called
   Vienna 5G System Level Simulator, allows one to trade accuracy for
   scale, thus enabling the evaluation of larger-scale networks in
   terms of average performance. This simulator is well tailored for
   the evaluation of lower-layer procedures, including
   signal-processing techniques. However, it cannot be used to
   evaluate multi-layer, end-to-end scenarios. On the other hand,
   Simu5G is a discrete-event, end-to-end, application-level
   simulator. You can model applications in it, and have endpoints
   communicate at the application layer with their packets traversing
   all the protocol layers, through arbitrarily complex network
   scenarios.

.. question:: How is Simu5G different from other end-to-end simulators,
   such as 5G LENA or 5G-air-simulator?

   Features that set Simu5G apart include:

   -  network-controlled device-to-device communications;
   -  Multi-access edge computing, with ETSI-compliant interfaces towards
      real MEC applications;
   -  EN-DC and NE-DC deployments and dual-stack communications;
   -  real-time emulation, with real applications and devices using the
      simulated 5G network.

.. question:: Can I integrate Simu5G with other models, e.g., run a
   simulation where a WiFi host communicates with a 5G UE?

   Simu5G is a *model library* for the OMNeT++ simulation
   framework. Any other model library written for OMNeT++ can be
   integrated with it. Notably, Simu5G makes extensive use of models
   from `the INET framework <https://inet.omnetpp.org>`__ (e.g.,
   routers and hosts).

.. question:: Is Simu5G compatible with SimuLTE? Can I run simulations
   with both 4G and 5G nodes?

   Yes and yes. Simu5G contains the LTE models of SimuLTE, so 4G and 5G nodes
   can be part of the same simulation. You can also run Dual Connectivity
   deployments: EN-DC (a 4G eNB acting as the master node and a 5G gNB as the
   secondary node) and NE-DC (a gNB master with an eNB secondary). See the
   :doc:`users-guide/overview` page.

.. question:: Does Simu5G model the control plane?

   Simu5G is a user-plane simulator. RRC functionality (bearer management,
   handover, radio link failure handling and RRC re-establishment) is
   modeled, but RRC signaling between the UE and the base station, and
   control-plane signaling towards the core network, are not simulated:
   procedures take effect through direct function calls between the modules
   involved, and where the duration of a
   procedure matters (handover, re-establishment), it is modeled with timers.
   Bearers come from configuration rather than from session management
   signaling.

.. question:: What are the other main modeling limitations?

   -  No MIMO support.
   -  The user plane is IPv4 only.
   -  Device-to-device communication is a research prototype, not based on
      the 3GPP sidelink specifications.
   -  There is no scheduling request: a UE without an uplink grant obtains one
      through the random access procedure.
   -  Logical channel prioritization at the UE has no prioritized bit rate.

.. question:: What hardware is required to run Simu5G as an emulator?

   An off-the-shelf desktop pc with two network interfaces can
   run an emulation of a multicell 5G network carrying application
   traffic up to several Mbps. See our demo in the
   :doc:`emulation <users-guide/emulation>` page.

.. question:: How do I get involved?

   Cool! You have some choices here:

   -  If you found a bug, or you want to contribute a fix or a new
      functionality, please open an issue or a pull request on
      `GitHub <https://github.com/Unipisa/Simu5G>`__. For larger
      contributions, please :doc:`contact us <contacts>` first.
   -  If you have any Simu5G-related research project or research
      article, and you want it to be acknowledged on this site,
      please :doc:`contact us <contacts>`.

   The Simu5G community is proud of you!
