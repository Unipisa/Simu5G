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


# delete namespace
sudo ip netns del ns1
