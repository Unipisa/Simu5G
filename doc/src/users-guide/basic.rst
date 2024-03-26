Introduction
============

Simu5G is a 5G New Radio (NR) simulator based on the OMNeT++ simulation
framework, and provides a collection of models with well-defined
interfaces, which can be instantiated and connected to build arbitrarily
complex simulation scenarios. Simu5G incorporates all the models from
the INET library, which allows one to simulate generic TCP/IP networks
including 5G NR layer-2 interfaces. In particular, Simu5G simulates the
data plane of the 5G Radio Access Network (RAN) and 5G Core (5GC)
network.

Simu5G extends its predecessor SimuLTE, hence it shares many of the
functionalities with it, as well as the general architecture and design
choices. For this reason, many of the classes mentioned throughout this
document have a name with the "Lte" prefix.

Before reading this document, it is warmly suggested that novice users
take a look at the OMNeT++ and INET documentation.

High-level architecture
=======================

Considering an end-to-end perspective, Simu5G is able to simulate all
the components in <TODO replace with figure with UE, gNB,
UPF, internet, server>. The element in grey are the ones that are
entirely realized using modules from the INET library. In other words,
Simu5G adds to the INET ecosystem the 4G/5G radio access network and
core network.

By composing the above modules, it is possible to implement any network
scenarios, possibly including multiple users, base stations, remote
servers and so on.

*User Equipments* (UEs) and *gNodeBs* (gNBs) are the main nodes of the
Radio Access Network (RAN), and are connected to Internet domain via the
5G Core (5GC) network. Since Simu5G only models the data plane of the
mobile network, the CN is represented by a set of *User Plane Functions*
(UPFs), whose scope is to route traffic between the gNBs and the
Internet. A variable number of UPFs can be present in a Simu5G network,
and one of them acts as the entry/exit point of the mobile network
domain.

As far as the RAN is concerned, gNBs can connect to each other via the
*X2 interface*. This is used for handover purposes (i.e., allowing a UE
to move from one serving gNB to another one), and can be employed for
interference coordination.

gNB can be organized in StandAlone (SA) or Non StandAlone (NSA)
deployments. In a SA deployment, gNBs are directly connected to the 5GC,
whereas with NSA the gNB is connected to a primary eNodeB (eNB) - i.e.,
a 4G (LTE) Base Station -, which is in turn connected to the Evolved
Packet Core (EPC) - i.e., the 4G Core Network. All the 5G data traffic
will need to traverse the eNB before reaching the gNB or the core
network. The latter deployment enables the so-called EUTRAN-NR Dual
Connectivity (ENDC).

Two special nodes, namely the Binder and CarrierAggregation modules, are
singleton module that provide information and functions that can be
accessed and used by all the other nodes in the network via method
calls.

Main nodes
----------

In this section, we present the main modules of the Simu5G architecture.

NRUe
~~~~

The *NRUe* module is described by the
`nodes/NR/NrUe.ned <https://github.com/Unipisa/Simu5G/blob/master/src/nodes/NR/NRUe.ned>`__
file. Its structure is depicted in Figure X. It is a compound module,
which includes all the modules implementing the protocol stack, from the
application layer to the data link and PHY layer. Specifically, it
includes:

-  a vector of application modules, which can be any INET compatible
   application. Such applications can be based either on TCP or UDP, and
   their number can be configured by setting the ``numApps`` paramater.

-  UDP, TCP and SCTP modules as transport-layer protocols, taken from
   INET (refer to ... for details);

-  IPv4 and IPv6 modules as network-layer protocols, taken from INET
   (refer to ... for details). Note: IPv6 is not fully supported at the
   moment;

-  *cellularNic* module, which represents the Network Interface Card
   (NIC) implementing the 5G NewRadio protocol stack. The NIC represents
   the main implementation effort of Simu5G, and will be discussed later
   in more detail.

Data packets sent by an application traverse all the protocol stack
until they reach the *cellularNic* module, which further processes them
according to the 5G NR protocol and sends them to the serving gNB.
Likewise, packets from the gNB are received by the *cellularNic* module
and flow upwards to the proper receiving application.

Moreover, loopback (lo) and Ethernet (eth) NICs are optionally
available. The former can be used for handling local traffic, whereas
the latter can be used to connect the UE to another host via a wired
link and for *emulation* purposes.

Finally, the *mobility* module defines how the UE moves within the
simulation floorplan. The mobility type can be easily configured from
the INI configuration file, and any INET mobility module is supported.
See ... for more detail on mobility.

In other words, a UE is a mobile Internet host, endowed with 5G
capabilities thanks to the *cellularNic* module, which allows the UE to
connect and exchange data with one gNB.

gNodeB
~~~~~~

The *gNodeB* module is described by the
`nodes/NR/gNodeB.ned <https://github.com/Unipisa/Simu5G/blob/master/src/nodes/NR/gNodeB.ned>`__
file. It is a compound module and its structure is depicted in Figure X.
On the RAN side, it includes a *cellularNic* module that implements the
gNB-side of the 5G NR protocol and allows it to send/receive packets
to/from all connected UEs over the radio channel (this will described
later in more detail), whereas IPv4 and IPv6 modules (taken from INET,
refer to ... for details) provide network-layer functionalities (Note:
IPv6 is not fully supported at the moment).

On the 5GC side, wired connectivity is realized via the Point-to-Point
Protocol (PPP) by the *pppIf* NIC. It is worth pointing out that data
forwarding within the 5GC is realized via GPRS Tunneling Protocol (GTP).
For this reason, packets received from UEs (i.e., from the *cellularNic*
module) need to be encapsulated within a GTP packet before sending them
through the *pppIf* NIC. Likewise, packets received from the 5GC (i.e.,
from the *pppIf* interface) need to be decapsulated in order to get the
original data packet before sending them to the *cellularNic* module.
The GTP encapsulation/decapsulation procedure is performed by the
*trafficFlowFilter* and *gtpUser* modules. More details on this will be
provided in the dedicated section.

Upf
~~~

The *Upf* module is described by the
`nodes/NR/Upf.ned <https://github.com/Unipisa/Simu5G/blob/master/src/nodes/Upf.ned>`__
file. Its main submodules are depicted in Figure X. The *Upf* module
basically works like a INET router (indeed, it inherits its structure),
as it includes vectors of different NIC types (such as PPP, ethernet and
wireless LAN) that allows one to connect the Upf module to any other
module using any kind of data link type.

Moreover, the *Upf* module includes the *trafficFlowFilter* and
*gtpUser* modules to enable GTP encapsulation/decapsulation, similar to
the gNodeB module. Again, more details on this can be found in the
dedicated section. These two modules are actually used by Upf only if it
acts as the entry/exit point of the 5GC (i.e., connects the 5G network
domain with the Internet domain).

Instead, when the Upf module is used to model an intermediate UPF
(iUPF), its behavior replicates the one of a classic router. In fact, it
receives GTP packets on a NIC and sends them to the Ipv4 module, which
in turn forwards them to the proper output NIC according to its
forwarding table.

Binder
~~~~~~

The *Binder* module is a simple module, defined in
`common/binder/Binder.ned <https://github.com/Unipisa/Simu5G/blob/master/src/nodes/NR/NRUe.ned>`__
file. The Binder is visible by every other node in the simulation and
stores information about them, such as references to nodes. It is used,
for instance, to locate the interfering gNBs in order to compute the
inter-cell interference perceived by a UE in its serving cell.

CarrierAggregation
~~~~~~~~~~~~~~~~~~

The *CarrierAggregation* module is used to configure the Carrier
Aggregation feature. It includes a vector of Component Carriers, each of
them including the corresponding relevant parameters. There must be one
(and one only) instance of the module in the network.

Network Interface Card modeling
-------------------------------

The core of Simu5G is the modeling of the 5G NewRadio protocol. The
latter defines layer 1 and layer 2 of the TCP/IP protocol stack, hence
we implemented by creating a specific NIC. Namely, the 5G NIC
functionalities are provided by the *NRNicGnb* and *NRNicUe* modules.
With reference to Figure X, they include all the layers of the NR
protocol stack, from the PDCP to the physical layer.

Example of data flow
--------------------

Use the table and tabular environments for basic tables — see
Table `1 <#tab:widgets>`__, for example. For more information, please
see this help article on
`tables <https://www.overleaf.com/learn/latex/tables>`__.

.. container::
   :name: tab:widgets

   .. table:: An example table.

      ======= ========
      Item    Quantity
      ======= ========
      Widgets 42
      Gadgets 13
      ======= ========

Physical layer modeling
=======================

Channel model
-------------

Channel status reporting
------------------------

MAC layer
=========

Scheduling
----------

Hybrid ARQ
----------

RLC layer
=========

PDCP layer
==========

X2
==

Handover procedure
==================

Handover selection algorithm
----------------------------

Handover management via the X2 interface
----------------------------------------

Dual connectivity
=================

Carrier aggregation
===================

Core network modeling
=====================

In particular, once an IP datagram is sent by the *cellularNic*, the
latter sends it to the *gtpUser* module that takes care of i) identify
the IP address of the next hop for the IP datagram within the 5GC, and
ii) add a GTP header to the packet. The resulting GTP packet is then
sent via the Udp and ipv4 modules to the node identified by the above IP
address. More details about the GTP tunneling procedure are provided in
the corresponding section.

