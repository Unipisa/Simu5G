:hide-footer:

Installation
============

Simu5G can be compiled on any platform supported by OMNeT++ and the INET
Framework.

Prerequisites
-------------

Simu5G requires `OMNeT++ <https://omnetpp.org>`__ and the `INET Framework
<https://inet.omnetpp.org>`__. The versions a given Simu5G release was tested
with, and is compatible with, are listed in that release's entry in the
`release notes <https://github.com/Unipisa/Simu5G/blob/master/WHATSNEW.md>`__.
Always check them before installing: Simu5G releases are tied to specific INET
versions.

If you install Simu5G with ``opp_env`` (see below), the matching OMNeT++ and
INET versions are installed automatically. Otherwise, install OMNeT++ and INET
first, following their own installation instructions, and make sure that they
work (e.g. try running some INET examples) before continuing.

Installing with opp_env (recommended)
-------------------------------------

`opp_env <https://github.com/omnetpp/opp_env>`__ installs Simu5G together with
the OMNeT++ and INET versions it needs. It supports Linux and macOS; on
Windows, it can be used in WSL2.

#. Install ``opp_env``, following its `installation instructions
   <https://github.com/omnetpp/opp_env/blob/main/INSTALL.md>`__.

#. Create an ``opp_env`` workspace in an empty directory:

   .. code:: bash

      opp_env init

#. Install Simu5G and its dependencies:

   .. code:: bash

      opp_env install simu5g-latest

   This will download and build OMNeT++, INET, and Simu5G. Use ``opp_env
   list`` to see the available versions; ``simu5g-git`` installs the current
   development version from the git repository.

#. Open a shell:

   .. code:: bash

      opp_env shell simu5g-latest

   Everything is built and the environment is set up, so you can run
   simulations right away (see :ref:`running-the-examples`). The IDE can also
   be started from this shell, with the ``omnetpp`` command.

Installing manually
-------------------

This section is for the case when OMNeT++ and INET are already installed (in
whatever way), and you obtained Simu5G separately, as a source archive or a git
clone. If you installed Simu5G with ``opp_env``, skip it: Simu5G has already
been built, and the environment is set up by ``opp_env shell``.

Obtaining Simu5G
~~~~~~~~~~~~~~~~

Source archives of the Simu5G releases are available on the `releases page
<https://github.com/Unipisa/Simu5G/releases>`__ on GitHub. Alternatively, you
can clone the `Simu5G repository <https://github.com/Unipisa/Simu5G>`__ to get
the most recent code.

.. note::

   The development branch may be unstable, and may require a different INET
   version than the latest release.

Building from the command line
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. Extract the Simu5G source archive (or clone the git repository), e.g. next
   to the INET directory.

#. Make sure that ``. setenv`` was executed both in the OMNeT++ and in the INET
   root directory.

#. Change to the Simu5G root directory, and type ``. setenv``. This sets
   ``SIMU5G_ROOT``, and adds Simu5G's ``bin`` directory to the ``PATH``.

#. Type ``make`` to build Simu5G in release mode, or ``make MODE=debug`` to
   build the debug version. The makefiles are generated automatically. INET
   must have been built in the same mode.

The debug build keeps the internal consistency checks (``ASSERT``) enabled,
which are compiled out of the release build. When developing or extending the
model, run your simulations with the debug build too.

Building from the OMNeT++ IDE
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. Extract the Simu5G source archive into your workspace directory, next to
   the INET directory.

#. Start the IDE, and make sure that the INET project is open and has been
   built.

#. Import the project using :menuselection:`File --> Import --> General -->
   Existing projects into Workspace`. Select the workspace directory as the
   root directory, and make sure that the "Copy projects into workspace" box
   is NOT checked. Click Finish.

#. Make sure that INET is referenced by the Simu5G project: right-click the
   Simu5G project, select :menuselection:`Properties --> Project References`,
   tick the INET project, and click "Apply and Close".

#. Build the project by pressing Ctrl+B (:menuselection:`Project --> Build
   All`).

Project Features
----------------

Parts of Simu5G are optional project features, which can be enabled or
disabled in the IDE (:menuselection:`Properties --> OMNeT++ --> Project
Features`) or with the ``opp_featuretool`` command in the Simu5G root
directory:

- **Simu5G D2D** (``Simu5G_D2D``, enabled by default): device-to-device
  communication support and the D2D examples.

- **Simu5G Cars** (``Simu5G_Cars``, disabled by default): 5G-enabled vehicular
  networks, which require Veins (see below).

Simulating 5G-enabled vehicular networks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Simu5G is able to simulate 5G communications in vehicular networks by
integrating `Veins <https://veins.car2x.org/>`__. Veins is a framework for
vehicular network simulation, based on the road traffic simulator `SUMO
<https://eclipse.dev/sumo/>`__. Refer to the Veins documentation for the Veins
and SUMO versions that match your INET version.

#. Install SUMO, following its instructions.

#. Download Veins, and extract it into your workspace, next to Simu5G and
   INET.

#. In the IDE, import the ``veins`` and ``veins_inet`` projects using
   :menuselection:`File --> Import --> General --> Existing projects into
   Workspace`, with the "Search for nested projects" box ticked.

#. Add ``veins_inet`` to the project references of Simu5G (right-click the
   Simu5G project, :menuselection:`Properties --> Project References`; do not
   tick ``veins``).

#. Enable the Simu5G Cars feature (:menuselection:`Properties --> OMNeT++ -->
   Project Features`, or ``opp_featuretool enable Simu5G_Cars``), and rebuild
   the project.

#. Launch SUMO through Veins: in the ``veins/bin`` directory, run
   ``./veins_launchd -vv -c <path-to-sumo>``. The ``-c`` option is not needed
   if SUMO's ``bin`` directory is on the ``PATH``.

#. Run one of the vehicular examples, ``simulations/nr/cars`` or
   ``simulations/lte/cars``, e.g. by selecting its folder in the IDE and
   clicking Run on the toolbar.

.. _running-the-examples:

Running the Examples
--------------------

Example simulations are under ``simulations/`` (grouped into ``lte/`` and
``nr/``), and emulation examples are under ``emulation/``.

- From the IDE: select an example folder or its ``omnetpp.ini`` file, and
  click Run on the toolbar.

- From the command line: change into an example directory, and run ``./run``,
  or invoke ``simu5g`` directly, e.g. ``simu5g -c <config-name> omnetpp.ini``.
  Add ``-u Cmdenv`` to run without the graphical user interface.
  ``simu5g_dbg`` runs the debug build.

The ``simu5g`` and ``simu5g_dbg`` scripts set up the NED path for Simu5G and
INET; they need ``SIMU5G_ROOT`` and ``INET_ROOT`` to be set (see ``setenv``).

Running the Tests
-----------------

In the Simu5G root directory:

- ``make tests`` runs the fingerprint tests. To run them with the debug build,
  use ``tests/fingerprint/fingerprints -d``.

- ``make unittests`` runs the unit tests; ``make MODE=debug unittests`` runs
  them against the debug build.
