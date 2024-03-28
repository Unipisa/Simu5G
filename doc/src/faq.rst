:hide-footer:

Frequently Asked Questions
==========================


.. question:: Where can I download the Simu5G code?

   Welcome on board! Visit :doc:`this page <install>` to
   download Simu5G.

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

   As far as we know, Simu5G is the only one to support the
   following features:

   -  network-controlled device-to-device communications (including
      Mode 1 of C-V2X);
   -  Multi-access edge computing;
   -  ENDC deployments and dual-stack communications.

.. question:: Can I integrate Simu5G with other models, e.g., run a
   simulation where a WiFi host communicates with a 5G UE?

   Simu5G is a *model library* for the OMNeT++ simulation
   framework. Any other model library written for OMNeT++ can be
   integrated with it. Notably, Simu5G makes extensive use of models
   from `the INET framework <https://inet.omnetpp.org>`__ (e.g.,
   routers and hosts).

.. question:: Is Simu5G compatible with SimuLTE? Can I run simulations
   with both 4G and 5G nodes?

   Yes and yes. More to the point, you can also run 5G
   simulations with ENDC deployments (i.e., a 4G eNB acting as a
   master and a 5G gNB acting as a slave). See the
   :doc:`users-guide/overview` page.

.. question:: What hardware is required to run Simu5G as an emulator?

   An off-the-shelf desktop pc with two network interfaces can
   run an emulation of a multicell 5G network carrying application
   traffic up to several Mbps. See our demo in the
   :doc:`emulation <users-guide/emulation>` page.

.. question:: How do I get involved?

   Cool! You have some choices here:

   -  If you want to develop new functionalities for Simu5G (or you
      already have) and you would like to integrate them in the
      repository, please :doc:`contact us <contacts>`
   -  If you have any Simu5G-related research project or research
      article, and you want it to be acknowledged on this site,
      please :doc:`contact us <contacts>`.

   The Simu5G community is proud of you!
