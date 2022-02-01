#!/bin/bash

# send a JSON file including simulation parameters (add -v option for details)
curl -XPOST -F "document=@/mnt/Data/giovanni/simu5G-inet4/simu5G/simulations/NR/digitalTwin_example/dt_interface/parameters.json" http://localhost:8080/parameters

# send a JSON file including simulation configurations (add -v option for details)
curl -XPOST -F "document=@/mnt/Data/giovanni/simu5G-inet4/simu5G/simulations/NR/digitalTwin_example/dt_interface/parameters.json" http://localhost:8080/simconfig


curl "http://localhost:8080/sim"

sleep 5

# send GET request to run simulations and obtain the metrics specified by the query string (add -v option for details)
# it will return a JSON-formatted text inside the body of the response 
curl "http://localhost:8080/sim?macCellThroughputDl"
