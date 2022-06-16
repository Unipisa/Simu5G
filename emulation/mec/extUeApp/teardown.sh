#!/bin/bash 
#   
#   +++++++++++++++++++++++++     +++++++++++++++++++++++++ 
#   +     Namespace ns1     +     +         Simu5G        +
#   +                       +     +                       +
#   +  UE App         [veth1]<--->[sim-veth1]             +
#   +            192.168.3.2+     +                       +
#   +                       +     +                       + 
#   +++++++++++++++++++++++++     +++++++++++++++++++++++++
#  

# delete namespaces
sudo ip netns del ns1
