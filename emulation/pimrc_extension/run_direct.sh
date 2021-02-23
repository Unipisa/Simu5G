#!/bin/bash

# This script runs the traffic without the emulated network in the middle (just using the loopback interface)

SENDERCMD="/home/giovanni/projects/emulation_sender/build/emulation_sender";	
RECEIVERCMD="/home/giovanni/projects/emulation_receiver/build/emulation_receiver";
OUTPUTFOLDER="baseline-direct"

# ===== CREATE VIRTUAL LINKS ===== #

# create virtual ethernet link: veth0 <--> veth1, veth2 <--> veth3 
sudo ip link add veth0 type veth peer name veth1
sudo ip link add veth2 type veth peer name veth3

# veth0 <--> veth1 link uses 192.168.2.x addresses; veth2 <--> veth3 link uses 192.168.3.x addresses
sudo ip addr add 192.168.2.2 dev veth1
sudo ip addr add 192.168.3.2 dev veth3

# bring up both interfaces
sudo ip link set veth0 up
sudo ip link set veth1 up
sudo ip link set veth2 up
sudo ip link set veth3 up

# add routes for new link
sudo route add -net 192.168.2.0 netmask 255.255.255.0 dev veth1
sudo route add -net 192.168.3.0 netmask 255.255.255.0 dev veth3
sudo route add -net 10.0.2.0 netmask 255.255.255.0 dev veth3     # enables backward path to the simulation  
sudo route add -net 10.0.3.0 netmask 255.255.255.0 dev veth1     # enables backward path to the simulation
  
sudo ethtool --offload veth1 rx off tx off # disable TCP checksum offloading to make sure that TCP checksum is actually calculated



# ===== START TRAFFIC ===== #

printf " --- START RECEIVER  --- \n\n";
$RECEIVERCMD -p udp &

printf "\n\n --- START SENDER --- \n\n";
$SENDERCMD -h 192.168.2.2 -p udp -s 1000 -t 0.04 -d 120 -o $OUTPUTFOLDER



# ===== DESTROY VIRTUAL LINKS ===== #

sudo ip link del veth0
sudo ip link del veth2