#!/bin/bash
#   
#   +++++++++++++++++++++++++     +++++++++++++++++++++++++ 
#   +     Namespace ns1     +     +         Simu5G        +
#   +                       +     +                       +
#   +  Server App     [veth1]<--->[sim-veth1]             +
#   +            192.168.2.2+     +                       + 
#   +                       +     +                       +
#   +++++++++++++++++++++++++     +++++++++++++++++++++++++
#   

# create namespace
sudo ip netns add ns1

# create virtual ethernet link: ns1.veth1 <--> sim-veth1
sudo ip link add veth1 netns ns1 type veth peer name sim-veth1

# Assign the address 192.168.2.2 with netmask 255.255.255.0 to `veth1`
sudo ip netns exec ns1 ip addr add 192.168.2.2/24 dev veth1

# bring up all interfaces
sudo ip netns exec ns1 ip link set veth1 up
sudo ip link set sim-veth1 up

# add default IP route within the new namespace 
sudo ip netns exec ns1 route add default dev veth1

# disable TCP checksum offloading to make sure that TCP checksum is actually calculated
sudo ip netns exec ns1 ethtool --offload veth1 rx off tx off 
