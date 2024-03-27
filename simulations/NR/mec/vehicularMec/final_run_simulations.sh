 #! /bin/bash 
echo "STARTING"
start=`date +%s`


#td
opp_runall ./run  -m -u Cmdenv -n ../../../../emulation:../../..:../../../../src:../../../../../inet/examples:../../../../../inet/showcases:../../../../../inet/src:../../../../../inet/tests/validation:../../../../../inet/tests/networks:../../../../../inet/tutorials:../../../../../veins/examples/veins:../../../../../veins/src/veins:../../../../../veins_inet/src/veins_inet:../../../../../veins_inet/examples/veins_inet --image-path=../../../../images:../../../../../inet/images:../../../../../veins/images:../../../../../veins_inet/images -l ../../../../src/simu5g -l ../../../../../inet/src/INET -l ../../../../../veins/src/veins -l ../../../../../veins_inet/src/veins_inet --debug-on-errors=true -c teleoperated_driving --cmdenv-redirect-output=false
tar -zcvf td.tar.gz logs
rm -rf logs
end=`date +%s`
echo "Execution time for td cars was `expr $end - $start` seconds.">> timer.txt
mv results results_td



#cs

opp_runall ./run  -m -u Cmdenv -n ../../../../emulation:../../..:../../../../src:../../../../../inet/examples:../../../../../inet/showcases:../../../../../inet/src:../../../../../inet/tests/validation:../../../../../inet/tests/networks:../../../../../inet/tutorials:../../../../../veins/examples/veins:../../../../../veins/src/veins:../../../../../veins_inet/src/veins_inet:../../../../../veins_inet/examples/veins_inet --image-path=../../../../images:../../../../../inet/images:../../../../../veins/images:../../../../../veins_inet/images -l ../../../../src/simu5g -l ../../../../../inet/src/INET -l ../../../../../veins/src/veins -l ../../../../../veins_inet/src/veins_inet --debug-on-errors=true -c cooperative_sensing --cmdenv-redirect-output=false
tar -zcvf cs.tar.gz logs
rm -rf logs
end=`date +%s`
echo "Execution time for cs cars was `expr $end - $start` seconds.">> timer.txt
mv results results_cs

#cm

opp_runall ./run  -m -u Cmdenv -n ../../../../emulation:../../..:../../../../src:../../../../../inet/examples:../../../../../inet/showcases:../../../../../inet/src:../../../../../inet/tests/validation:../../../../../inet/tests/networks:../../../../../inet/tutorials:../../../../../veins/examples/veins:../../../../../veins/src/veins:../../../../../veins_inet/src/veins_inet:../../../../../veins_inet/examples/veins_inet --image-path=../../../../images:../../../../../inet/images:../../../../../veins/images:../../../../../veins_inet/images -l ../../../../src/simu5g -l ../../../../../inet/src/INET -l ../../../../../veins/src/veins -l ../../../../../veins_inet/src/veins_inet --debug-on-errors=true -c cooperative_maneuver --cmdenv-redirect-output=false

tar -zcvf cm.tar.gz logs
rm -rf logs
end=`date +%s`
echo "Execution time for cm cars was `expr $end - $start` seconds.">> timer.txt
mv results results_cm

#ca

opp_runall ./run  -m -u Cmdenv -n ../../../../emulation:../../..:../../../../src:../../../../../inet/examples:../../../../../inet/showcases:../../../../../inet/src:../../../../../inet/tests/validation:../../../../../inet/tests/networks:../../../../../inet/tutorials:../../../../../veins/examples/veins:../../../../../veins/src/veins:../../../../../veins_inet/src/veins_inet:../../../../../veins_inet/examples/veins_inet --image-path=../../../../images:../../../../../inet/images:../../../../../veins/images:../../../../../veins_inet/images -l ../../../../src/simu5g -l ../../../../../inet/src/INET -l ../../../../../veins/src/veins -l ../../../../../veins_inet/src/veins_inet --debug-on-errors=true -c cooperative_awarness --cmdenv-redirect-output=false
tar -zcvf ca.tar.gz logs
rm -rf logs
end=`date +%s`
echo "Execution time for ca cars was `expr $end - $start` seconds.">> timer.txt
mv results results_ca




end=`date +%s`
echo "Execution time  was `expr $end - $start` seconds.">> timer.txt