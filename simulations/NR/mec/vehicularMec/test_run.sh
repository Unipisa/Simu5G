 #! /bin/bash 

start=`date +%s`

 opp_run_dbg   -m -u Cmdenv -n ../../../../emulation:../../..:../../../../src:../../../../../inet/examples:../../../../../inet/showcases:../../../../../inet/src:../../../../../inet/tests/validation:../../../../../inet/tests/networks:../../../../../inet/tutorials:../../../../../veins/examples/veins:../../../../../veins/src/veins:../../../../../veins_inet/src/veins_inet:../../../../../veins_inet/examples/veins_inet --image-path=../../../../images:../../../../../inet/images:../../../../../veins/images:../../../../../veins_inet/images -l ../../../../src/simu5g -l ../../../../../inet/src/INET -l ../../../../../veins/src/veins -l ../../../../../veins_inet/src/veins_inet --debug-on-errors=true -c teleoperated_driving_2356230 --cmdenv-redirect-output=false  

end=`date +%s`
echo Execution time was `expr $end - $start` seconds.
