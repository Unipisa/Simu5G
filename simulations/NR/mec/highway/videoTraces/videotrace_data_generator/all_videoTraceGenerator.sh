#!/bin/bash
cd videoTraces
for ue in {0..20}
    do
        for rep in {0..15}
            do
               python3 ../traceDataGenerator.py -f 30 -t 300  -d 0 -o videoTrace_ue[$ue]_rep_$rep
        done
done
