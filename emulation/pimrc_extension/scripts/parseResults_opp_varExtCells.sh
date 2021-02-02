
# ===== EXPORT VECTORS ===== #
echo "--> Exporting RLC delay DL..."

for NUMEXTCELLS in 0 1 2 3 4 5 6; do
        scavetool export -f 'module(*.realUe.lteNic.nrRlc.um) AND name(rlcDelayDl:vector)' -F 'CSV-S' -T v -o rlcDelayDl-extCells_${NUMEXTCELLS}.csv ../results/evalExtCells/*.vec
done

