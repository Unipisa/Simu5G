***********************************
*      RNIS TEST SIMULATION       *
***********************************

The simulations/NR/mec/rnisTest folder includes a simulation example that shows how to configure 
and use the Radio Network Information Service.

The RnisTest configuration in the omnetpp.ini file represents a scenario with one UE in a single-cell
network, endowed with one MEC Host. The MEC Platform within the MEC Host provides the RNIService.

The UE runs the UeRnisTestApp locally and interacts with the MecRnisTestApp residing at the MEC Host.
The implementation of the applications can be found in apps/mec/RnisTestApp folder.

At the beginning of the simulation, the UE requests to MEC Orchestrator to instantiate a new 
MecRnisTestApp. Upon receiving an acknowledgement indicating that the MEC app has been deployed, the
UeRnisTestApp sends a "start" message to the MecRnisTestApp, triggering a periodic request for 
information to the RNIService from the MEC app.
The RNIService replies with a message in JSON format including the L2Measurements, as specified by
the ETSI MEC specifications about the RNIS API. Each response obtained by the MEC app is forwarded
to the UE app, which in turn prints it out to the Qtenv log (and to a file, if the logger flag
has been set).con  

==========================