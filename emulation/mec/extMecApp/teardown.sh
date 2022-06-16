#!/bin/bash 
#   
#   +++++++++++++++++++++++++     +++++++++++++++++++++++++ 
#   +     Namespace ns1     +     +         Simu5G        +
#   +                       +     +                       +
#   +  MEC App        [veth1]<--->[sim-veth1]             +
#   +            192.168.2.2+     +                       +
#   +                       +     +                       + 
#   +++++++++++++++++++++++++     +++++++++++++++++++++++++
#  

# delete namespaces
sudo ip netns del ns1
