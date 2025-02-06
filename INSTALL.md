INSTALLATION INSTRUCTIONS
=========================

The Simu5G Framework can be compiled on any platform supported by the INET Framework.

Prerequisites
-------------

You should have a

- working OMNeT++ v6.0.3 (or later) installation. (Download from http://omnetpp.org)
- working INET-Framework (v4.5) installation. (Download from http://inet.omnetpp.org)

Make sure your OMNeT++ installation works OK (e.g. try running the samples)
and it is in the path (to test, try the command "which nedtool"). On
Windows, open a console with the "mingwenv.cmd" command. The PATH and other
variables will be automatically adjusted for you. Use this console to compile
and run INET and Simu5G.

Install and test INET according to the installation instructions found in the archive.
Be sure to check if the INET examples are running fine before continuing.

Installing Simu5G using opp_env
-------------------------------

1. Make sure that [opp_env](https://github.com/omnetpp/opp_env) is correctly
   installed on your machine (only macOS and Linux are supported,
   but you can also use `opp_env` in a Windows WSL2 VM just fine).

2. Create an opp_env workspace in an empty directory with the command
   `opp_env init`

3. Install Simu5G and its dependencies (inc. OMNeT++) with the command
   `opp_env install simu5g-latest`

4. Open a development shell with the command `opp_env shell simulte-latest`
   then start the IDE from there or continue working from the command line.

Building Simu5G from the IDE (automatically)
--------------------------------------------

1. Make sure that the INET project is open and correctly built in your workspace.

2. In `Help | Install Simulation Models...`, select the `Simu5G` project and click
   `Install Project`.

3. Make sure that INET is correctly referenced in the project by right-clicking on
   the Simu5G folder and selecting `Properties | Project References`. Tick the
   "inet4.5" box and click "Apply and Close".

Building Simu5G from the IDE (manually)
---------------------------------------

1. Extract the downloaded Simu5G tarball next to the INET directory
   (i.e. into your workspace directory, if you are using the IDE).

2. Start the IDE, and ensure that the 'inet4.5' project is open and correctly built.

3. Import the project using: `File | Import | General | Existing projects into Workspace`.
   Then select the workspace dir as the root directory, and be sure NOT to check the
   "Copy projects into workspace" box. Click Finish.

4. Make sure that INET is correctly referenced in the project by right-clicking on
   the Simu5G folder and selecting `Properties | Project References`. Tick the
   "inet4.5" box and click "Apply and Close".

5. You can build the project by pressing CTRL-B (Project | Build all)

6. To run an example from the IDE, select the simulation example's folder under 
   'simulations/NR', and click 'Run' on the toolbar.

Building Simu5G from the command line (manually)
------------------------------------------------

1. Extract the downloaded Simu5G tarball next to the INET directory.

2. Make sure that `. setenv` was executed both for OMNeT++ and INET.

3. Change to the Simu5G directory.

4. Type `. setenv`. This will add the `simu5G/bin` directory to the PATH environment variable.

5. Type `make makefiles`. This should generate the `src/Makefile`.

6. Type "make" to build the Simu5G executable (release version). Use "`make MODE=debug`
   to build debug version.

7. You can run examples by changing into a directory under 'simulations/NR', and
   executing `./run`

Enjoy,
The Simu5G Team
