To integrate a MEC system into an LTE/5G network the following modules needs to be used:
- the UALCM proxy
- the MEC orchestrator
- a desired number of MEC hosts

The UALCM proxy must be connected to the MEC orchestrator via the "toMecOrchestrator/fromMecOrchestrator" gates.
MEC host associated to the MEC system are selected through the "mecHostList" parameter of the MEC orchestrator (since more MEC system can be deployed). Also the MEC platform manager of each MEC host needs to know its MEC orchestrator, so the "mecOrchestrator" parameter must be configured with name of the relative MEC orchestrator module. Finally, eNB/gNBs associated via the MEC hosts are selected with the "eNBList" MEC host's paramater, or if the MEC hosts are directly connected to the base station, they consider the active gates connected to the them, without setting the "eNBList" parameter.
Each MEC host is configured with a maximum amount of resources that are used by the MEC orchestrator to select which MEC host should run the required MEC app:

```
*.mecHost.maxMECApps = 100		 # max ME Apps to instantiate
*.mecHost.maxRam = 32GB			 # max KBytes of Ram 
*.mecHost.maxDisk = 100TB		 # max KBytes of Disk Space 
*.mecHost.maxCpuSpeed = 400000	 # max CPU
```

Once the MEC entities are configured, the last thing to set up is relative to the MEC services running on the MEC platform module of a MEC host, if they are needed.
The "numMecServices" parameter selects the number of MEC services running on the MEC platform module and then, each MEC service is configured as follows:

```
*.mecHost.mecPlatform.numMecServices = 1
*.mecHost.mecPlatform.mecService[0].typename = "RNIService"
*.mecHost.mecPlatform.mecService[0].localAddress = "mecHost.mecPlatform"
*.mecHost.mecPlatform.mecService[0].localPort = 10020
```

The demo scenarios in the singleMecHost and MultiMecHost folders describes the configuration to set up 5G network with a MEC system

The UALCM proxy interfaces with the UE trough the Device App, tipically running in the UE itself, in addition to the UE application communicating with the MEC app. 


