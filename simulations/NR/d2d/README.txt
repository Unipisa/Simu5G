Detailed information on the modeling of D2D in SimuLTE/Simu5G can be found in the paper:
A. Virdis, G. Nardini, G. Stea, "Modeling unicast device-to-device communications with SimuLTE", IWSLS2 2016, Vienna, July 1st, 2016


*****************************
* How to simulate D2D flows *
*****************************

=== D2D peering association between UEs ===
By default, D2D-capable UEs still communicate using the traditional cellular mode (e.g. via the gNodeB). 
You need to enable the setup of a D2D peering-map by enabling the "d2dInitialMode".

*.ue[*].cellularNic.d2dInitialMode = true


=== D2D Transmission Power ===
It is possible to use different transmission power for UL and D2D transmissions, by setting
the "d2dTxPower" parameter in the UE.
E.g.: *.ue[0].cellularNic.phy.ueTxPower = 26   # UL Tx Power (in dB)
      *.ue[0].cellularNic.phy.d2dTxPower = 20  # D2D Tx Power (in dB)


=== CQI for D2D tranmissions ===
The CQI used for D2D transmissions can be selected either statically or dynamically.
In the former case, the CQI is fixed at the beginning of the simulation and it is employed for all
D2D transmissions. In the latter case, the CQI is computed for every D2D link (according to the
"d2dPeerAddresses" parameter) and reported to the gNodeB, which uses it for scheduling resources.
To enable static CQI, you need to set the following parameters:
    **.usePreconfiguredTxParams = true
    **.d2dCqi = 7
To enable CQI reporting, you need to set the following parameter:
    *.gnb.cellularNic.phy.enableD2DCqiReporting = true
If you set both flags, static CQIs will be used. In that case, CQI reporting will be useless, so we suggest 
to disable it in order to save computational time. 
    

=== AmcPilot module for D2D ===
The AmcPilot is the module that is responsible for selecting transmission parameters given the channel conditions of nodes.
A new AmcPilot module has been added to select transmission parameter of D2D links. 
You need to explicitly specify it in the ini file as follows:
    **.amcMode = "D2D"
    
    
=== Dynamic Mode Switching === 
The "D2DModeSelection" module, located at the gNodeB, is responsible to periodically select the communication mode for
each pair of D2D peering UEs, according to the implemented policy. To enable dynamic mode selection:
    *.gnb.cellularNic.d2dModeSelection = true
A simple policy that selects the mode having the best channel quality is implemented by the "D2DModeSelectionBestCqi"
module. The latter is obtained by extending the "D2DModeSelection" module. You can always add your own policy by
extending the "D2DModeSelection" module. You can select the policy for the current simulation in the ini as follows:
    *.gnb.cellularNic.d2dModeSelectionType = "D2DModeSelectionBestCqi"
where you need to indicate the name of your new module.    
When the gNodeB trigeers a mode switch for a given flow, it sends a notification to the transmitting endpoint of that 
flow. The message traverses the whole protocol stack up to the PDCP layer. At each layer, it is possible to define 
specific behavior for the switch handler.

**************************
* D2D Simulation folder  *
**************************
The "simulations/NR/d2d" folder contains some examples to test D2D communications (omnetpp.ini).

1) One pair of UEs
    a) "SinglePair-UDP-Infra"
    b) "SinglePair-UDP-D2D"
    c) "SinglePair-TCP-Infra"
    d) "SinglePair-TCP-D2D"
2) N pairs of UEs
    a) "MultiplePairs-UDP-Infra"
    b) "MultiplePairs-UDP-D2D"
    c) "MultiplePairs-TCP-Infra"
    d) "MultiplePairs-TCP-D2D"
3) Dynamic Mode Switching
    a) "SinglePair-modeSwitching-UDP"
    b) "SinglePair-modeSwitching-TCP"        

