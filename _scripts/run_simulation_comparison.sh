#!/bin/bash

set -e  # Exit on first error

# Configuration
SIM_PATH="simulations/nr/mec/multiOperator/"
SIM_ARGS="-c MultiOperator_SingleMec -r 0"
SIMTIME_LIMIT=5s

run_simulation() {
    local commit=$1
    local results_suffix=$2

    echo "Processing commit: $commit"
    git checkout $commit
    make cleanall && make makefiles && make MODE=debug -j12
    cd $SIM_PATH
    simu5g_dbg $SIM_ARGS --sim-time-limit=$SIMTIME_LIMIT \
       -u Cmdenv --cmdenv-express-mode=false --cmdenv-redirect-output=true --result-dir=$results_suffix --print-undisposed=false \
       --cmdenv-log-prefix='#%e t=%t %C %u > ' --cmdenv-log-simulation=true \
       --cmdenv-output-file='${resultdir}/cmdenv-raw.out' --output-scalar-file='${resultdir}/out.sca' || true
    cd - > /dev/null
}

# Run simulations
run_simulation "before" "results-before"
run_simulation "after" "results-after"

# Strip ANSI coloring sequences from the logs
cat "$SIM_PATH/results-before/cmdenv-raw.out" | ansifilter -T | sed 's/, id=[0-9]*/, id=REDACTED/' > "$SIM_PATH/results-before/cmdenv.out"
cat "$SIM_PATH/results-after/cmdenv-raw.out"  | ansifilter -T | sed 's/, id=[0-9]*/, id=REDACTED/' > "$SIM_PATH/results-after/cmdenv.out"

# Diff the results
echo "diff -u $SIM_PATH/results-before/cmdenv.out  $SIM_PATH/results-after/cmdenv.out | less"
echo "diff -u $SIM_PATH/results-before/out.sca  $SIM_PATH/results-after/out.sca | less"

echo "Done."
