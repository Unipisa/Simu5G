
# ===== EXPORT VECTORS ===== #
echo "--> Exporting RTT..."

for RBS in 10 25 50 100; do
    for NUMEROLOGY in 0 1 2 3 4; do
        scavetool export -f 'module(*.server.tcpApp[0]) AND name(requestResponseAppRtt:vector)' -F 'CSV-S' -T v -o baseline-simulation-5G-u_${NUMEROLOGY}-rbs_${RBS}.csv ../results/BaselineSimulation5G/*numerology=${NUMEROLOGY}*numRBs=${RBS}-*.vec
    done
done

