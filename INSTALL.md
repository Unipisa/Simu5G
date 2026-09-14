INSTALLATION INSTRUCTIONS
=========================

Simu5G can be compiled on any platform supported by OMNeT++ and the INET
Framework.

Prerequisites
-------------

Simu5G requires OMNeT++ (https://omnetpp.org) and the INET Framework
(https://inet.omnetpp.org). The versions a given Simu5G release was tested
with, and is compatible with, are listed in that release's entry in
WHATSNEW.md. Always check it before installing: Simu5G releases are tied to
specific INET versions.

If you install Simu5G with opp_env (see below), the matching OMNeT++ and INET
versions are installed automatically. Otherwise, install OMNeT++ and INET
first, following their own installation instructions, and make sure that
they work (e.g. try running some INET examples) before continuing.

Installing Simu5G using opp_env (recommended)
---------------------------------------------

opp_env (https://github.com/omnetpp/opp_env) installs Simu5G together with
the OMNeT++ and INET versions it needs. It supports Linux and macOS; on
Windows, it can be used in WSL2.

1. Install opp_env, following its instructions.

2. Create an opp_env workspace in an empty directory with the command
   `opp_env init`.

3. Install Simu5G and its dependencies with the command
   `opp_env install simu5g-latest`. This will download and build OMNeT++,
   INET, and Simu5G. Use `opp_env list` to see the available versions;
   `simu5g-git` installs the current development version from the git
   repository.

4. Open a shell with the command `opp_env shell simu5g-latest`. Everything
   is built and the environment is set up, so you can run simulations right
   away (see "Running the examples" below). The IDE can also be started
   from this shell, with the `omnetpp` command.

Building Simu5G from the command line
-------------------------------------

This section, and the next one, are for the case when OMNeT++ and INET are
already installed (in whatever way), and you obtained Simu5G separately, as
a source archive or a git clone. If you installed Simu5G with opp_env,
skip them: it has already been built, and the environment is set up by
`opp_env shell`.

1. Extract the Simu5G source archive (or clone the git repository), e.g.
   next to the INET directory.

2. Make sure that `. setenv` was executed both in the OMNeT++ and in the
   INET root directory.

3. Change to the Simu5G root directory, and type `. setenv`. This sets
   `SIMU5G_ROOT`, and adds Simu5G's `bin` directory to the `PATH`.

4. Type `make` to build Simu5G in release mode, or `make MODE=debug` to
   build the debug version. The makefiles are generated automatically.
   INET must have been built in the same mode.

The debug build keeps the internal consistency checks (`ASSERT`) enabled,
which are compiled out of the release build. When developing or extending
the model, run your simulations with the debug build too.

Building Simu5G from the IDE
----------------------------

1. Extract the Simu5G source archive into your workspace directory, next to
   the INET directory.

2. Start the IDE, and make sure that the INET project is open and has been
   built.

3. Import the project using `File | Import | General | Existing projects into
   Workspace`. Select the workspace directory as the root directory, and
   make sure that the "Copy projects into workspace" box is NOT checked.
   Click Finish.

4. Make sure that INET is referenced by the Simu5G project: right-click the
   Simu5G project, select `Properties | Project References`, tick the INET
   project, and click "Apply and Close".

5. Build the project by pressing Ctrl+B (`Project | Build All`).

Project features
----------------

Parts of Simu5G are optional project features, which can be enabled or
disabled in the IDE (`Properties | OMNeT++ | Project Features`) or with the
`opp_featuretool` command in the Simu5G root directory:

- **Simu5G D2D** (`Simu5G_D2D`, enabled by default): device-to-device
  communication support and the D2D examples.

- **Simu5G Cars** (`Simu5G_Cars`, disabled by default): 5G-enabled vehicular
  networks, which require Veins (https://veins.car2x.org) and its
  `veins_inet` subproject. To use it, import the `veins` and `veins_inet`
  projects into the workspace, add `veins_inet` to the project references
  of Simu5G, and enable the feature (e.g. `opp_featuretool enable
  Simu5G_Cars`). The `cars` examples also need the SUMO road traffic
  simulator (https://eclipse.dev/sumo/), launched through Veins'
  `veins-launchd` script before the simulation is started. Refer to the
  Veins documentation for the Veins and SUMO versions that match your INET
  version.

Running the examples
--------------------

Example simulations are under `simulations/` (grouped into `lte/` and
`nr/`), and emulation examples are under `emulation/`.

- From the IDE: select an example folder or its `omnetpp.ini` file, and click
  Run on the toolbar.

- From the command line: change into an example directory, and run
  `./run`, or invoke `simu5g` directly, e.g.
  `simu5g -c <config-name> omnetpp.ini`. Add `-u Cmdenv` to run without the
  graphical user interface. `simu5g_dbg` runs the debug build.

The `simu5g` and `simu5g_dbg` scripts set up the NED path for Simu5G and
INET; they need `SIMU5G_ROOT` and `INET_ROOT` to be set (see `setenv`).

Running the tests
-----------------

In the Simu5G root directory:

- `make tests` runs the fingerprint tests (see `tests/fingerprint/README`).
  To run them with the debug build, use `tests/fingerprint/fingerprints -d`.
- `make unittests` runs the unit tests (see `tests/unit/README`);
  `make MODE=debug unittests` runs them against the debug build.

Generating the documentation
----------------------------

`make neddoc` generates the NED reference documentation of the model.
