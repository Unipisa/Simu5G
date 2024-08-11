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

#ifndef _SPLITBEARERSTABLE_H_
#define _SPLITBEARERSTABLE_H_

#include "common/LteCommon.h"

namespace simu5g {

/**
 * @class SplitBearersTable
 * @brief Hash table to keep track of connections
 *
 * This is a hash table used by the RRC layer
 * to assign CIDs to different connections.
 * The table is in the format:
 *  _____________________________________________
 * | srcAddr | dstAddr | typeOfService | number
 *
 * A 4-tuple is used to check if the connection was already
 * established and return the number of sent packets; otherwise, a
 * new entry is added to the table.
 */
class SplitBearersTable
{
  public:
    SplitBearersTable();
    virtual ~SplitBearersTable();

    /**
     * find_entry() checks if an entry is in the
     * table and, if found, increments the number and returns it
     * @return number of number fields in hash table:
     *             - -1 if no entry was found
     *             - number if it was found
     */
    int find_entry(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService);

    /**
     * create_entry() adds a new entry to the table
     */
    int create_entry(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService);

  private:
    /**
     * hash_func() calculates the hash function used
     * by this structure. At the moment it's simply an OR
     * operation between all fields of the 3-tuple
     */
    unsigned int hash_func(uint32_t srcAddr, uint32_t dstAddr, uint16_t typeOfService);

    /*
     * Data Structures
     */

    /**
     * \struct entry
     * \brief hash table entry
     *
     * This structure contains an entry of the
     * connections hash table. It contains
     * all fields of the 3-tuple and the
     * associated LCID (Logical Connection ID).
     */
    struct entry_
    {
        bool present_;
        uint32_t srcAddr_;
        uint32_t dstAddr_;
        uint16_t typeOfService_;
        int number_;
    };

    /// This is the maximum number of allowed connections * 2
    static constexpr int TABLE_SIZE = 2048;

    /// Hash table of size TABLE_SIZE
    entry_ ht_[TABLE_SIZE];
};

} //namespace

#endif

