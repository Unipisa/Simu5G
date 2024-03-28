:hide-footer:

Installation
============

Prerequisites
-------------

Simu5G can be compiled on any platform supported by OMNeT++ and
INET frameworks.

You should have working `OMNeT++ <https://omnetpp.org>`__ and
`INET Framework <https://inet.omnetpp.org>`__ installations. See
the required versions above, depending on the Simu5G version
you selected.

Make sure your OMNeT++ installation works OK (e.g. try running
the samples) and it is in the PATH environment variable (to
test it, type the command :command:`which nedtool` into the terminal).
On Windows, open a console with the :command:`mingwenv.cmd` command. The
PATH and other variables will be automatically adjusted for
you. Use this console to compile and run INET and Simu5G.

Install and test INET according to the installation
instructions found in the archive. Be sure to check if the INET
examples are running fine before continuing.

Obtaining Simu5G
----------------

Latest release: **Simu5G 1.2.2**.

In order to get Simu5G on your machine, you have the following
options:

#. `Download and install Simu5G directly from the OMNeT++
   IDE <#install_from_ide>`__
#. `Download the source code and build Simu5G
   manually <#build_source_code>`__
#. `Download and run Simu5G Plug-and-Play. <#download_vm>`__

Download and install Simu5G from the OMNeT++ IDE
------------------------------------------------

This is the easiest way to get Simu5G:

#. Open the OMNeT++ IDE on your machine and make sure that the
   'inet4.5' project is open and correctly built in your
   workspace
#. In the menu bar, click Help \| Install Simulation Models...
   \| Select Simu5G 1.2.2 and click Install Project
#. Wait until the building process is complete. This may take
   up several minutes.

| `Back to top <#guide>`__

Download and build the source code
----------------------------------

Download the Simu5G tarball including the latest stable release
(`zip <https://github.com/Unipisa/Simu5G/archive/refs/tags/v1.2.2.zip>`__,
`tar.gz <https://github.com/Unipisa/Simu5G/archive/refs/tags/v1.2.2.tar.gz>`__).
This version requires `OMNeT++
v6.0 <https://github.com/omnetpp/omnetpp/releases/tag/omnetpp-6.0.1>`__
and `INET
v4.5 <https://github.com/inet-framework/inet/releases/tag/v4.5.0>`__.
For previous releases, visit `this
link <https://github.com/Unipisa/Simu5G/releases>`__.

Alternatively, you can download the most recent code directly
from the `Simu5G repository on
GitHub <https://github.com/Unipisa/Simu5G>`__. Current master
branch requires `OMNeT++
v6.0 <https://github.com/omnetpp/omnetpp/releases/tag/omnetpp-6.0.1>`__
and `INET
v4.5 <https://github.com/inet-framework/inet/releases/tag/v4.5.0>`__.

.. note:: 
   
   This is the development branch, hence this version of
   the code might be *unstable*.

Building from the OMNeT++ IDE
-----------------------------

#. Extract the downloaded Simu5G tarball next to the "inet4.5"
   directory (i.e. into your workspace directory, if you are
   using the IDE).
#. Start the IDE, and ensure that the 'inet4.5' project is open
   and correctly built.
#. Import the project using: File \| Import \| General \|
   Existing projects into Workspace. Then select the workspace
   dir as the root directory, and be sure NOT to check the
   "Copy projects into workspace" box. Click Finish.
#. You can build the project by pressing CTRL-B (Project \|
   Build all)
#. To run an example from the IDE, select the simulation
   example's folder under 'simulations', and click 'Run' on the
   toolbar.

Building from the command line
------------------------------

#. Extract the downloaded Simu5G tarball next to the "inet4.5"
   directory
#. Change to the Simu5G directory.
#. Type ". setenv". This will add the Simu5g/bin directory to
   the PATH environment variable.
#. Type "make makefiles". This should generate the makefiles.
#. Type "make" to build the Simu5G executable (debug version).
   Use "make MODE=release" to build release version.
#. Run an examples by changing into one of the directories
   under 'simulations/NR', and executing "./run"


Simu5G Plug-and-Play
--------------------

If you cannot wait to try Simu5G... COOL! You are in the right
place!

You can download the following Virtual Machine appliance,
import it into your favorite virtualization software (e.g.
Oracle Virtualbox, VMWare Player), and start simulating! You do
not need to carry out the complete installation procedure, we
already did the boring stuff for you ;)

.. rubric:: Download `Simu5G 1.2.1
   PnP <https://unipiit-my.sharepoint.com/:u:/g/personal/a018358_unipi_it/Eakokdmky4xKjxaHdU_VRIgBruh12GYAfsXw3saZExrdxw?e=mMh2Mv>`__
   :name: download-simu5g-1.2.1-pnp

This VM includes Ubuntu 20.04, with OMNeT++ 6.0, INET 4.4.0,
**Simu5G 1.2.1**. We also included `Veins
5.2 <https://veins.car2x.org/>`__ in case you want to simulate
5G-enabled vehicular networks.

.. rubric:: How to import the VM
   :name: how-to-import-the-vm

This guide assumes you have `Oracle Virtualbox
6.1 <https://www.virtualbox.org/>`__ installed on your machine.

#. Open Virtualbox, then click File -> Import Appliance...
#. In the dialog window, browse your file system and select the
   downloaded file (with *.ova* extension). Then, click Next.
#. Check the VM configuration, then click Import.
#. Select the new VM on the left side of the Virtualbox main
   window, then click Start.

Running a simulation
~~~~~~~~~~~~~~~~~~~~

#. From the main screen of the VM, click on the OMNeT++ icon on
   the left bar. This will launch the OMNeT++ IDE.
#. In the project explorer, on the left side, select Simu5G >
   simulations > NR.
#. Select one of the examples and click 'Run' on the toolbar.
#. Enjoy! :)

.. note:: 
   
   If you want to run the 'cars' example, you first
   need to launch SUMO by clicking the corresponding icon in
   the left bar.

Simulating 5G-enabled vehicular networks
----------------------------------------

Simu5G is able to simulate 5G communications in vehicular
networks by integrating `Veins <https://veins.car2x.org/>`__.
Veins is a framework for vehicular networks simulation, based
on the road traffic simulator
`SUMO <https://sumo.dlr.de/docs/index.html>`__.

To simulate vehicular networks within Simu5G, you have two
options:

-  Get the `Simu5G 1.2.1 PnP <simu5g-pnp.html>`__ Virtual
   Machine;
-  Download Veins and integrate it manually within your Simu5G
   project. If you choose this option, read the following
   instructions.

.. rubric:: Integrating Veins into your Simu5G project
   :name: integrating-veins-into-your-simu5g-project

This guide assumes you already have a working version of Simu5G
on your machine. If you don't, please get it before going
ahead. See the `Install Guide <install.html>`__.

.. rubric:: Download SUMO
   :name: download-sumo

#. Download `SUMO
   v1.11.0 <https://sourceforge.net/projects/sumo/files/sumo/version%201.11.0/>`__
#. Extract the archive's content, e.g.,
   PATH_TO_SUMO/sumo-1.11.0
#. Build SUMO by following the instructions in the README file
   included in the package

.. rubric:: Download Veins
   :name: download-veins

#. Download `Veins 5.2 <https://veins.car2x.org/download/>`__
#. Extract the archive's content into your workspace (e.g. next
   to Simu5G and inet4.4 directories)

.. rubric:: Import Veins into your workspace using the IDE
   :name: import-veins-into-your-workspace-using-the-ide

#. Start the OMNeT++ IDE and open your Simu5G workspace
#. Import the project using: File \| Import \| General \|
   Existing projects into Workspace. Then select your workspace
   directory, i.e. the one including Simu5G, inet4.4 and Veins
   folders. Tick the "Search for nested projects" box and
   select "veins" and "veins_inet" projects. Click Finish.
#. In the project explorer, right-click on the "Simu5G" folder
   \| Properties \| Project References. Tick the "veins_inet"
   box (do not tick the "veins" folder. Click OK.
#. In the project explorer, right-click on the "Simu5G" folder
   \| Properties \| OMNeT++ \| Project Features. Tick the
   "Simu5G Cars" box. Click OK.
#. Build the project by pressing CTRL-B (or Project \| Build
   all).

.. rubric:: Run a simulation example
   :name: run-a-simulation-example

#. In your terminal, navigate to the "veins/bin" folder and
   launch SUMO, by using the command *./veins-launchd.py -vv -c
   PATH_TO_SUMO/sumo-1.11.0/bin/sumo* (replace PATH_TO_SUMO
   with the actual directory where you extracted SUMO. The -c
   option is not necessary if you added
   "PATH_TO_SUMO/sumo-1.11.0/bin/" to your PATH environment
   variable)
#. In the OMNeT++ IDE, select the 'Simu5G/simulations/NR/cars'
   example folder, and click 'Run' on the toolbar.