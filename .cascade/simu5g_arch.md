# Simu5G architecture

## Control Plane / Data Plane separation

We want to push Simu5G in the direction of VERY CLEAR Control Plane / Data Plane
separation, as mandated by 3GPP specs.

1. Control Plane in NIC: RRC and its submodules like BearerManagement. The task
   is to CONFIGURE the Data Plane (incl setup/teardown of entities). Processing
   data packets is NOT  Control Plane functionality, and STRICTLY FORBIDDEN!
   Moreover, in Simu5G the Control Plane is only implemented in its
   functionality, but NOT the Control Plane communication protocol. Control
   Plane entities communicate via C++ method calls, NOT by sending packets.
   Adding gates to BearerManagement is a strict NO-NO!!

2. Data Plane. Roughly speaking, that's everything else outside RRC in the NIC.
   Its task is processing data packets, SOLELY. Any code that does configuration
   changes does NOT belong there. For example, deleteEntities() methods are NOT
   supposed to be there at all. It is the task of the Control Plane, likely
   BearerManagement.
