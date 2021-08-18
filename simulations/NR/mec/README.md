To integrate a MEC architecture into a LTE/5G network the following modules needs to be used:
- UALCM proxy
- MEC orchestrator (one per MEC system) 
- a configurable number of MEC hosts

The UALCM proxy must be connected to the MEC orchestrator via the "toMecOrchestrator/fromMecOrchestrator" gates.
MEC host associated to the MEC system are selected through the "mecHostList" parameter of the MEC orchestrator. Also the MEC platform manager of each MEC host needs to know its MEC orchestrator, so the "mecOrchestrator" parameter must be configured with name of the relative MEC orchestrator module. Finally, eNB/gNBs associated via the MEC hosts are selected with the "eNBList" MEC host's paramater, or if the MEC host are directly connected to the base station, they consider the active gates connected to the them.


Once the MEC entities are configured, the last thing to set is relative to the MEC services running on the MEC platform module of a MEC host, if are needed.
The "numMecServices" parameter selects the number of MEC services running on the MEC platform module and then set the port where the service listens for MEC apps requests.

The network, and the relative MEC-related configuration, summerises what has just been described.

// METTERE RETE 

// METTERE LA CONFIGURAZIONE


The UALCM proxy interfaces with the UE trough the Device App, tipically running in the UE itself, in addition to the UE application communicating with the MEC app. 


