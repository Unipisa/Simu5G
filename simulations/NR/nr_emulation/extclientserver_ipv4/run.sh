#!/bin/bash
# make sure you run '. setenv' in the Simu5G root directory before running this script

# create tun devices
sudo ip tuntap add mode tun dev tun0
sudo ip tuntap add mode tun dev tun1

# enable tun devices
sudo ip link set dev tun0 up
sudo ip link set dev tun1 up

# assign IP addresses to interfaces
sudo ip addr add 10.0.3.1/24 dev tun0
sudo ip addr add 10.0.0.1/24 dev tun1

# add IP routes to the simulation
sudo route add -net 10.0.2.0 netmask 255.255.255.0 dev tun1   # client -> simulation
sudo route add -net 10.0.3.0 netmask 255.255.255.0 dev tun0   # server -> simulation

# wait one second to prevent simulation crashes (don't know why though...)
echo "Setting up TUN interfaces and IP routes..."
sleep 5

# run simulation
simu5g -u Cmdenv -c ExtClientServer

# destroy tun devices
sudo ip link del tun0
sudo ip link del tun1