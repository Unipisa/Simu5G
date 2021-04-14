INSTALLATION INSTRUCTIONS
=========================

The Simu5G Framework can be compiled on any platform supported by the INET Framework.


Prerequisites
-------------

You should have a 
- working OMNeT++ (v5.6.2) installation. (Download from http://omnetpp.org)
- working INET-Framework (v4.2.2) installation. (Download from http://inet.omnetpp.org)

Make sure your OMNeT++ installation works OK (e.g. try running the samples)
and it is in the path (to test, try the command "which nedtool"). On
Windows, open a console with the "mingwenv.cmd" command. The PATH and other
variables will be automatically adjusted for you. Use this console to compile
and run INET and Simu5G.

Install and test INET according to the installation instructions found in the archive.
Be sure to check if the INET examples are running fine before continuing.


Building Simu5G from the IDE
-----------------------------

1. Extract the downloaded Simu5G tarball next to the INET directory
   (i.e. into your workspace directory, if you are using the IDE).

2. Start the IDE, and ensure that the 'inet4' project is open and correctly built.

3. Import the project using: File | Import | General | Existing projects into Workspace.
   Then select the workspace dir as the root directory, and be sure NOT to check the
   "Copy projects into workspace" box. Click Finish.

4. You can build the project by pressing CTRL-B (Project | Build all)

5. To run an example from the IDE, select the simulation example's folder under 
   'simulations/NR', and click 'Run' on the toolbar.


Building Simu5G from the command line
--------------------------------------

1. Extract the downloaded Simu5G tarball next to the INET directory.

2. Change to the Simu5G directory.

3. Type ". setenv". This will add the 'simu5G/bin' directory to the PATH environment variable.

4. Type "make makefiles". This should generate the makefiles.

5. Type "make" to build the Simu5G executable (release version). Use "make MODE=debug"
   to build debug version.

6. You can run examples by changing into a directory under 'simulations/NR', and 
   executing "./run"


Enjoy, 
The Simu5G Team
