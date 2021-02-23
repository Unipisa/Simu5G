#!/bin/bash

SNDINT=40
OUTPUTFOLDER="stats-backgroundUes"

if [[ $1 != "noexport" ]] 
then

    # ===== EXPORT SCALARS ===== #
    echo "--> Exporting scalars..."
    for BKUES in `seq 0 25 200`; do
	# option -T must be set to s if scalar, v if vector, t if statistics 
	    opp_scavetool export -f 'module =~ "*.ue.app[0]" AND name =~ "rcvdPkLifetime:stats"' -F 'CSV-S' -T t -o ${OUTPUTFOLDER}/sca_rtt_bkUEs=${BKUES}-generated.csv results/BgTrafficScalabilityEval-generated/*sndInt=40ms-bkUEs=${BKUES}-*.sca
        opp_scavetool export -f 'module =~ "*.ue.app[0]" AND name =~ "rcvdPkLifetime:stats"' -F 'CSV-S' -T t -o ${OUTPUTFOLDER}/sca_rtt_bkUEs=${BKUES}-simulated.csv results/BgTrafficScalabilityEval-simulated/*sndInt=40ms-bkUEs=${BKUES}-*.sca

        opp_scavetool export -f 'module =~ "*.gnb.lteNic.mac" AND name =~ "avgServedBlocksUl:mean"' -F 'CSV-S' -T s -o ${OUTPUTFOLDER}/sca_blocksUl_bkUEs=${BKUES}-generated.csv results/BgTrafficScalabilityEval-generated/*sndInt=40ms-bkUEs=${BKUES}-*.sca

        opp_scavetool export -f 'module =~ "*.gnb.lteNic.mac" AND name =~ "avgServedBlocksUl:mean"' -F 'CSV-S' -T s -o ${OUTPUTFOLDER}/sca_blocksUl_bkUEs=${BKUES}-simulated.csv results/BgTrafficScalabilityEval-simulated/*sndInt=40ms-bkUEs=${BKUES}-*.sca

        opp_scavetool export -f 'module =~ "*.gnb.lteNic.mac" AND name =~ "avgServedBlocksDl:mean"' -F 'CSV-S' -T s -o ${OUTPUTFOLDER}/sca_blocksDl_bkUEs=${BKUES}-generated.csv results/BgTrafficScalabilityEval-generated/*sndInt=40ms-bkUEs=${BKUES}-*.sca

        opp_scavetool export -f 'module =~ "*.gnb.lteNic.mac" AND name =~ "avgServedBlocksDl:mean"' -F 'CSV-S' -T s -o ${OUTPUTFOLDER}/sca_blocksDl_bkUEs=${BKUES}-simulated.csv results/BgTrafficScalabilityEval-simulated/*sndInt=40ms-bkUEs=${BKUES}-*.sca

    done
    echo "--> done"
fi
# ==================== #



# ==== PARSE FILES ==== #

echo "--> Parsing extracted scalars..."


for METRIC in rtt blocksUl blocksDl; do
	# define output file
	OUTPUTFILE=${OUTPUTFOLDER}/avg_${METRIC}.csv

	# print header
	HEADER="v Background UEs | Scenario >\tBackgroundUEs\tBackgroundUEs-conf\tSimulatedUEs\tSimulatedUEs-conf\n"
	printf "$HEADER" > $OUTPUTFILE

	for BKUES in `seq 0 25 200`; do
            printf "${BKUES}" >> $OUTPUTFILE
            for SCENARIO in generated simulated; do                           
	        # get avg and conf int
	        INPUTFILE="${OUTPUTFOLDER}/sca_${METRIC}_bkUEs=${BKUES}-${SCENARIO}.csv"
	        printf "\t" >> $OUTPUTFILE

                if [ $METRIC = rtt ]; then
		    ./scripts/processStats_backgroundUes.pl -file $INPUTFILE >> $OUTPUTFILE
                else
                    ./scripts/processScalar_backgroundUes.pl -file $INPUTFILE >> $OUTPUTFILE
                fi
            done
	    printf "\n" >> $OUTPUTFILE
	done
done

echo "--> done"
# ==================== #

# ===== PLOT ALL ======= #
#echo "--> Plotting charts..."
#./plot.gpl 
# ====================== #

echo "--> End"
