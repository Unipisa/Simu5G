//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/pdcp_rrc/ConnectionsTable.h"

namespace simu5g {

ConnectionsTable::ConnectionsTable()
{
    // Table is reset by putting all fields equal to 0xFF
    memset(ht_, 0xFF, sizeof(struct entry_) * TABLE_SIZE);
}

unsigned int ConnectionsTable::hash_func(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService)
{
    return (typeOfService | srcAddr | dstAddr) % TABLE_SIZE;
}

unsigned int ConnectionsTable::hash_func(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService, uint16_t dir)
{
    return (typeOfService | srcAddr | dstAddr | dir) % TABLE_SIZE;
}

LogicalCid ConnectionsTable::find_entry(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService)
{
    int hashIndex = hash_func(srcAddr, dstAddr, typeOfService);
    while (true) {
        if (ht_[hashIndex].lcid_ == 0xFFFF)                                                        // Entry not found
            return 0xFFFF;
        if (ht_[hashIndex].srcAddr_ == srcAddr &&
            ht_[hashIndex].dstAddr_ == dstAddr &&
            ht_[hashIndex].typeOfService_ == typeOfService)
            return ht_[hashIndex].lcid_;                                                          // Entry found
        hashIndex = (hashIndex + 1) % TABLE_SIZE;    // Linear scanning of the hash table
    }
}

LogicalCid ConnectionsTable::find_entry(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService, uint16_t dir)
{
    int hashIndex = hash_func(srcAddr, dstAddr, typeOfService, dir);
    while (true) {
        if (ht_[hashIndex].lcid_ == 0xFFFF)                                                        // Entry not found
            return 0xFFFF;
        if (ht_[hashIndex].srcAddr_ == srcAddr &&
            ht_[hashIndex].dstAddr_ == dstAddr &&
            ht_[hashIndex].typeOfService_ == typeOfService &&
            ht_[hashIndex].dir_ == dir)
            return ht_[hashIndex].lcid_;                                                          // Entry found
        hashIndex = (hashIndex + 1) % TABLE_SIZE;    // Linear scanning of the hash table
    }
}

void ConnectionsTable::create_entry(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService, LogicalCid lcid)
{
    int hashIndex = hash_func(srcAddr, dstAddr, typeOfService);
    while (ht_[hashIndex].lcid_ != 0xFFFF)
        hashIndex = (hashIndex + 1) % TABLE_SIZE;                                                       // Linear scanning of the hash table
    ht_[hashIndex].srcAddr_ = srcAddr;
    ht_[hashIndex].dstAddr_ = dstAddr;
    ht_[hashIndex].typeOfService_ = typeOfService;
    ht_[hashIndex].lcid_ = lcid;
}

void ConnectionsTable::create_entry(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService, uint16_t dir, LogicalCid lcid)
{
    int hashIndex = hash_func(srcAddr, dstAddr, typeOfService, dir);
    while (ht_[hashIndex].lcid_ != 0xFFFF)
        hashIndex = (hashIndex + 1) % TABLE_SIZE;                                                       // Linear scanning of the hash table
    ht_[hashIndex].srcAddr_ = srcAddr;
    ht_[hashIndex].dstAddr_ = dstAddr;
    ht_[hashIndex].typeOfService_ = typeOfService;
    ht_[hashIndex].dir_ = dir;
    ht_[hashIndex].lcid_ = lcid;
}


} //namespace

