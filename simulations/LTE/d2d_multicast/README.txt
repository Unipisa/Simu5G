***************
* What's new? *
***************
Support for *** one-to-many *** device-to-device (D2D) communications has been added to SimuLTE.
Main features are listed below:
- Registration of UEs to an IP multicast group
- Broadcast transmission at PHY layer for UEs and filtering (at the receiver's side) based on the multicast group ID
- Suppression of H-ARQ feedback for 1:M D2D communications 
- New application (AlertSender/Receiver) for sending/receiving alert messages to UEs belonging to a multicast group 

This is an experimental version for D2D communications. Any feedback and/or suggestion is highly appreciated.


**************************************************
* How to simulate one-to-many D2D communications *
**************************************************

=== Enable D2D for both UEs and eNB === 
For further information, see simulations/d2d/README.txt
E.g.:
    *.eNodeB.d2dCapable = true
    *.ueD2D[*].d2dCapable = true
    **.amcMode = "D2D"

=== Join a multicast group ===
UEs that need to transmit/receive data to/from a multicast group must join an IP multicast group.
To do this, configure the "demo.xml" file.
E.g.:
    <multicast-group hosts="ueD2D[*]" interfaces="cellular" address="224.0.0.10"/>
    
=== Sending data to a multicast group ===
Set the above multicast IP address as destination address of senders' application
E.g.
	# One-to-Many traffic between UEs (ueD2D[0] --> ueD2D[1..49])
	# Transmitter
	*.ueD2D[0].udpApp[*].typename = "AlertSender"
	*.ueD2D[0].udpApp[*].localPort = 3088+ancestorIndex(0) 
	*.ueD2D[0].udpApp[*].startTime = uniform(0s,0.02s)
	*.ueD2D[0].udpApp[*].destAddress = "224.0.0.10"      # IP address of the multicast group 
	*.ueD2D[0].udpApp[*].destPort = 1000
	
	# Receivers
	*.ueD2D[1..49].udpApp[*].typename = "AlertReceiver"
	*.ueD2D[1..49].udpApp[*].localPort = 1000

**************************
* D2D Simulation folder  *
**************************
The "simulations/d2d_multicast" folder contains examples to test 1:M D2D communications (omnetpp.ini).

1) One transmitter and two (close) receivers
    -) "D2DMulticast-1to2"
2) One transmitter and M receivers, randomly deployed within the cell
    -) "D2DMulticast-1toM"
    