//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//
import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;

namespace simu5g;

class LteRac extends inet::FieldsChunk
{
    // meaningful only for DL (response) RAC packets : if true RAC request has been admitted.
    bool success;
    // preamble index chosen by UE (0..numPreambles-1); used for collision detection at eNB
    int preambleIndex = -1;
    chunkLength = inet::B(1); // TODO: size 0
}
