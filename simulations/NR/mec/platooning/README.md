We simulate a scenario where group of seven cars moves along the road in a straight line. The motorway is served by two Mobile Network Operators (A and B), each one having four gNodeBs co-located. Three cars, named A[\*], belong to MNO A, whereas the other four, B[\*], belong to MNO B.
Each MNO has its own MEC system, including a single MEC host. A MEC federation allows the communication between the two MEC systems, so that platooning controllers can
manage platoons having cars connected to different MNOs. All the cars will be associated to the same Controller App to form a single platoon.
In the .ini file there is a short comment over each configuration.

The modules involved in this scenario (Plaas framework modules) can be found in the src/apps/mec/PlatooningApp ([link](https://github.com/giovanninardini/Simu5G/tree/Plaas_framework/src/apps/mec/PlatooningApp)) folder.
