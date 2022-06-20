ExtClientServer Emulation example
---------------------------------

In this example the network consists of one UE connected to one gNB, one UPF, 
one router, one external (real) server and one external (real) client.

The external client application (i.e. associated to the UE in the emulation) sends 10MB 
using TCP to the external server application, traversing the emulated 5G network.

IP addresses and routing tables are set up by using mrt files (see "routing" folder).

Communication between the simulator and the host OS is realized using namespaces linked
together via virtual ethernet (veth) interfaces. 
In particular, we configure the following scenario:

+++++++++++++++++++++++++     +++++++++++++++++++++++++     +++++++++++++++++++++++++
+     Namespace ns1     +     +         Simu5G        +     +     Namespace ns2     +
+                       +     +                       +     +                       +
+  Client App     [veth1]<--->[sim-veth1]   [sim-veth2]<--->[veth2]     Server App  +
+            192.168.2.2+     +                       +     +192.168.3.2            +
+                       +     +                       +     +                       +
+++++++++++++++++++++++++     +++++++++++++++++++++++++     +++++++++++++++++++++++++

Packets sent needs to be smaller than the MTU of the veth interface (1500 bytes by default)

 
How to build and run the emulation 
----------------------------------

1) Make sure that the emulation feature is enabled in the INET project:

- via the IDE: right-click on the 'inet' folder in the Project Explorer -> Properties;
               select OMNeT++ -> Project Features; 
               tick the box "Network emulation support".
- via the command line: in the root INET folder, type 'opp_featuretool enable NetworkEmulationSupport'.

  Recompile INET with the command 'make makefiles && make' (in the root INET folder).  


2) In order to be able to send/receive packets through sockets, set the application permissions (specify
   the path of your OMNeT installation): 

    sudo setcap cap_net_raw,cap_net_admin=eip path/to/opp_run
    sudo setcap cap_net_raw,cap_net_admin=eip path/to/opp_run_dbg
    sudo setcap cap_net_raw,cap_net_admin=eip path/to/opp_run_release


3) Compile Simu5G from the command line by running (in the root Simu5G folder):
    
    $ . setenv
    $ make makefiles
    $ make MODE=release 
	 	

4) Setup the environment by typing:

    ./setup.sh	 
	 	
5) Run the external server for the uplink traffic, e.g. use "iperf" with the following command (type
   "man iperf" for options description):
  	    
  	(in a new terminal window) sudo ip netns exec ns1 iperf -s -i 1 -p 10021
  	
6) Run the simulation by typing:

    ./run.sh
   
7) Run the external server for the downlink traffic, e.g. use "iperf" with the following command (type
   "man iperf" for options description):
  	    
    (in a new terminal window) sudo ip netns exec ns2 iperf -c 192.168.2.2 -i 1 -p 10021 -M 536.0B -n 10.0M
   
        
8) Clean the environment by typing:

    ./teardown.sh
    
