#!/bin/bash
# make sure you run '. setenv' in the Simu5G root directory before running this script

# create tun device
sudo ip tuntap add mode tun dev tun0

# enable tun device
sudo ip link set dev tun0 up

# assign IP addresses to tun device (same as router in the simulation)
sudo ip addr add 10.0.2.1/24 dev tun0

# add route for backward path to the simulation (UE's address in the simulation)
sudo route add -net 10.0.0.0 netmask 255.255.255.0 dev tun0 

# run simulation
simu5g -u Cmdenv -c ExtServer_Tun

# destroy tun
sudo ip link del tun0


