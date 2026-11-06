#!/bin/bash
#
# Replicated A/B runs of two builds of Simu5G (see README).
#
# usage: run.sh <rootA> <rootB> <configs> <outdir> [<seeds> [<jobs>]]
#
#   <rootA>, <rootB>  Simu5G roots (e.g. two git worktrees), each built in
#                     debug mode
#   <configs>         a file with one "<dir>|<config>|<sim-time-limit>" line per
#                     configuration, <dir> relative to the root
#   <outdir>          where the results go: <outdir>/{A,B,A2}/<dir>__<config>/r<n>/
#   <seeds>           runs per side (default 30)
#   <jobs>            simulations in parallel (default 8)
#
# Side A runs 0..seeds-1 at rootA, side B the same runs at rootB, and side A2
# runs seeds..2*seeds-1 at rootA: the null control. Each run uses its run
# number as its seed set (the default seed-set = ${repetition}, with
# --repeat=2*seeds).
#
set -e
A=$1; B=$2; CONFIGS=$3; OUT=$4; N=${5:-30}; J=${6:-8}
[ -n "$OUT" ] || { echo "usage: run.sh <rootA> <rootB> <configs> <outdir> [<seeds> [<jobs>]]"; exit 1; }
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd)  # the runs write from their configurations' directories
REPEAT=$((2 * N))

runone() { # <root> <side> <dir> <config> <simtime> <run>
    local rd="$OUT/$2/$(echo "$3" | tr / _)__$4"
    mkdir -p "$rd/logs"
    ( cd "$1/$3" && SIMU5G_ROOT="$1" nice -n 19 "$1/bin/simu5g_dbg" -u Cmdenv -f omnetpp.ini -c "$4" \
        --repeat=$REPEAT -r "$6" --sim-time-limit="$5" \
        --result-dir="$rd/r$6" --output-scalar-file='${resultdir}/run.sca' --output-vector-file='${resultdir}/run.vec' \
        --vector-recording=false --cmdenv-express-mode=true --cmdenv-performance-display=false > "$rd/logs/r$6.log" 2>&1 )
    echo "$2 $3 $4 $6 exit=$?" >> "$OUT/$2.status"
}
export -f runone; export OUT REPEAT

for ((r = 0; r < N; r++)); do
    while IFS='|' read -r d c t; do
        [ -z "$d" ] && continue
        echo "$A A $d $c $t $r"
        echo "$B B $d $c $t $r"
        echo "$A A2 $d $c $t $((r + N))"
    done < "$CONFIGS"
done | xargs -P "$J" -L 1 bash -c 'runone "$@"' _

failed=$(cat "$OUT"/*.status | grep -vc 'exit=0' || true)
echo "done; runs that did not exit with 0: $failed"
