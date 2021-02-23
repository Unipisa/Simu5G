#! /bin/bash

################################################################################
# The script takes as input a UE's routing file (having address 10.0.0.*) and  #
# generates N new files assigning incremental IP address                       #
################################################################################

inputfile=$1                                     # the file to be replicated
repetitions=$2                                   # the number of replications

n=2

# Replication of the script, assigning new seed and run numbers
for (( i=0; i<$repetitions; i++ ))
do
    outputfile="bkUe$i.mrt"                   # name for the new file
    cp $inputfile $outputfile                    # copy the file

    # replace -seed option, if any
	sed -i 's/-seed 0/-seed '$i'/g' $outputfile  

    # find run numbers and replace them
    while read line
    do
        if [[ "$line" =~ "inet_addr" ]]
        then
            newaddr="10.0.0.$n"
            runnumber=`echo $line | cut -d ' ' -f 3`   # find the run number
            newrunnumber=$(($runnumber + $i))          # get the new run number
            
            # replace -r option
            sed -i 's/10.0.0.*/'$newaddr'/g' $outputfile       
        fi   
    done < $inputfile
    
    n=$(($n + 1))
done
