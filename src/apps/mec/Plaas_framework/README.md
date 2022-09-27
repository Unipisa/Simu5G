# Platooning as a Service (PlaaS)

Plaas is a framework complaint with ETSI MEC standard that allows to test centralized platooning control algorithms in multiple mobile network operators (MNOS) scenarios.
Cars, i.e. UE, can discover, join and leave platoons that are managed by MEC controller application running on MEC hosts.
With this framework is possible to:
- Test lateral and longitudinal platoon controllers
- Exploit ETSI MEC Location Service to retrieve positions and speed of the car involved in the platoon
- Evalute platoon control algorithms in terms of network and mec host computation capabilities

The framework is composed of 4 components:
- **UE Platooning App**: it runs on the vehicles and interacts with the PlaaS framework running on the MEC, by sending requests join or leave a platoon and receiving accelaration values from the Platoon Controller App to maintain the desired inter-vehicle distance and speed.
- **MEC Consumer App**: it is the digital twin of the UE app on the MEC, and interacts with the Producer App and, when associated to a platoon, with the Controller App. It forward commands (i.e. acceleration) from the MEC Controller App to the UE platooning app.
- **MEC Producer App**: it handles the platoon discovery requests from the Consumer Apps by identifying the platoon the latter may request to join.
- **MEC Controller App**: it manages a platoon. Internally it has two control algorithm: one, the Cruise Control Algorithm, is used to mantain the platoon stability. The second one, the Maneuver Control Algorithm, is in charge of perform join and leave maneuvers upon request reception. Both computes the accelaration values of the vehicles involved in the platoon sent then to the UE Platooning App (via the MEC Consumer App).

The control algorithms running in the MEC Controller App and the platoon selection algorithm can be implemented by inheriting the *PlatoonControllerBase* and *PlatoonSelectionBase* classes, respectively.

Some configurations of a scenario are already provided and can be found in the simulations/NR/mec/platooning ([link](https://github.com/giovanninardini/Simu5G/tree/Plaas_framework/simulations/NR/mec/platooning)) folder.



