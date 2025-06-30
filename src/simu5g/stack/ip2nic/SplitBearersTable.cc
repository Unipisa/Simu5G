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

#include "stack/ip2nic/SplitBearersTable.h"

namespace simu5g {

SplitBearersTable::SplitBearersTable()
{
    // Table is reset by putting all fields equal to 0x00
    memset(ht_, 0x0, sizeof(struct entry_) * TABLE_SIZE);
}

unsigned int SplitBearersTable::hash_func(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService)
{
    return (typeOfService | srcAddr | dstAddr) % TABLE_SIZE;
}

int SplitBearersTable::find_entry(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService)
{
    int hashIndex = hash_func(srcAddr, dstAddr, typeOfService);
    while (true) {
        if (!ht_[hashIndex].present_)                                                  // Entry not found
            return -1;
        if (ht_[hashIndex].srcAddr_ == srcAddr &&
            ht_[hashIndex].dstAddr_ == dstAddr &&
            ht_[hashIndex].typeOfService_ == typeOfService)
            return ++ht_[hashIndex].number_;                                                                // Entry found
        hashIndex = (hashIndex + 1) % TABLE_SIZE;    // Linear scanning of the hash table
    }
}

int SplitBearersTable::create_entry(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService)
{
    int hashIndex = hash_func(srcAddr, dstAddr, typeOfService);
    while (ht_[hashIndex].present_)
        hashIndex = (hashIndex + 1) % TABLE_SIZE;                                                       // Linear scanning of the hash table
    ht_[hashIndex].present_ = true;
    ht_[hashIndex].srcAddr_ = srcAddr;
    ht_[hashIndex].dstAddr_ = dstAddr;
    ht_[hashIndex].typeOfService_ = typeOfService;
    ht_[hashIndex].number_ = 0;
    return ht_[hashIndex].number_;
}

SplitBearersTable::~SplitBearersTable()
{
    memset(ht_, 0x00, sizeof(struct entry_) * TABLE_SIZE);
}

} //namespace

