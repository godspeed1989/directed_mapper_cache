#include "bram_cache.h"

#include <stdbool.h>
#include <string.h>

static void copy_block_data(uint8_t dst[DATA_WIDTH_B], uint8_t src[DATA_WIDTH_B])
{
    int i;
    for (i = 0; i< DATA_WIDTH_B; i++)
	#pragma HLS UNROLL
    {
        dst[i] = src[i];
    }
}

// Cache映射表
// 每个Cache块包含_N_块数据
static map_tbl_t    cache_map_tbl0 [CAHCE0_NUM_BLOCKS][_N_];
static map_tbl_t    cache_map_tbl1 [CAHCE1_NUM_BLOCKS][_N_];
static map_tbl_t    cache_map_tbl2 [CAHCE2_NUM_BLOCKS][_N_];
static map_tbl_t    cache_map_tbl3 [CAHCE3_NUM_BLOCKS][_N_];

// Cache数据
// 每个Cache块包含_N_块数据
static uint8_t      cache_data_store0 [CAHCE0_NUM_BLOCKS][_N_][DATA_WIDTH_B];
static uint8_t      cache_data_store1 [CAHCE1_NUM_BLOCKS][_N_][DATA_WIDTH_B];
static uint8_t      cache_data_store2 [CAHCE2_NUM_BLOCKS][_N_][DATA_WIDTH_B];
static uint8_t      cache_data_store3 [CAHCE3_NUM_BLOCKS][_N_][DATA_WIDTH_B];

const map_tbl_t zero_tbl_entry = {
    .ref_cnt = 0,
    .is_used = 0,
    .grpnum = 0,
};

static void reset_map_tbl (map_tbl_t cache_map_tbl[][_N_], int num_blocks)
{
    int i;
    for (i = 0; i < num_blocks; i++)
    #pragma HLS UNROLL
    {
        cache_map_tbl[i][0] = zero_tbl_entry;
        cache_map_tbl[i][1] = zero_tbl_entry;
        cache_map_tbl[i][2] = zero_tbl_entry;
        cache_map_tbl[i][3] = zero_tbl_entry;
    }
}

static void reset_map_tbls()
{
#pragma HLS DATAFLOW
    reset_map_tbl(cache_map_tbl0, CAHCE0_NUM_BLOCKS);
    reset_map_tbl(cache_map_tbl1, CAHCE1_NUM_BLOCKS);
    reset_map_tbl(cache_map_tbl2, CAHCE2_NUM_BLOCKS);
    reset_map_tbl(cache_map_tbl3, CAHCE3_NUM_BLOCKS);
}

/**
 * query and copy from cache pool
 */
static void query_and_copy_block (uint16_t coffset, uint16_t cgrpnum,
                                  bool *hit_sel_addr, uint8_t rd_wr_data[DATA_WIDTH_B],
                                  map_tbl_t cache_map_tbl[][_N_],
                                  uint8_t cache_data_store[][_N_][DATA_WIDTH_B])
{
    int i;
    *hit_sel_addr = false;
    for (i = 0; i < _N_; i++)
    #pragma HLS PIPELINE
    {
        if (cache_map_tbl[coffset][i].is_used &&
            cache_map_tbl[coffset][i].grpnum == cgrpnum)
        {
            cache_map_tbl[coffset][i].ref_cnt++;
            copy_block_data(rd_wr_data, cache_data_store[coffset][i]);
            *hit_sel_addr = true;
            return;
        }
    }
}

/**
 * use new data to update cache pool
 */
static void update_cache_data (uint16_t coffset, uint16_t cgrpnum,
                               uint8_t rd_wr_data[DATA_WIDTH_B],
                               map_tbl_t cache_map_tbl[][_N_],
                               uint8_t cache_data_store[][_N_][DATA_WIDTH_B])
{
    int i;
    uint8_t idx, ref_cnt;

    /* case hit: update cache data */
    for (i = 0; i < _N_; i++)
    #pragma HLS UNROLL
    {
        if (cache_map_tbl[coffset][i].is_used &&
            cache_map_tbl[coffset][i].grpnum == cgrpnum)
        {
            copy_block_data(cache_data_store[coffset][i], rd_wr_data);
            return;
        }
    }

    /* case miss: pool has empty */
    for (i = 0; i < _N_; i++)
    #pragma HLS UNROLL
    {
        if (!cache_map_tbl[coffset][i].is_used)
        {
            cache_map_tbl[coffset][i].is_used = 1;
            copy_block_data(cache_data_store[coffset][i], rd_wr_data);
            cache_map_tbl[coffset][i].grpnum = cgrpnum;
            cache_map_tbl[coffset][i].ref_cnt = 1;
            return;
        }
    }

    /* case miss: pool is full */
    ref_cnt = 0xFF;
    for (i = 0; i < _N_; i++)
    #pragma HLS UNROLL
    {
        if (cache_map_tbl[coffset][i].ref_cnt < ref_cnt)
        {
            idx = i;
            ref_cnt = cache_map_tbl[coffset][i].ref_cnt;
            if (ref_cnt == 0)
                break;
        }
    }
    copy_block_data(cache_data_store[coffset][idx], rd_wr_data);
    cache_map_tbl[coffset][idx].grpnum = cgrpnum;
    cache_map_tbl[coffset][idx].ref_cnt = 1;

}

void bram_cache (
    /** Input signals **/
    _IN_    bool        type,
    _IN_    bool        reset,

    _IN_    uint32_t    addr0,
    _IN_    uint32_t    addr1,
    _IN_    uint32_t    addr2,
    _IN_    uint32_t    addr3,
    _IN_    uint32_t    addr4,

    /** type = QUERY_CACHE   的输出 **/
    /** type = UPDATE_CACHE  的输入 **/
    _INOUT_     bool    *hit_sel_addr0,
    _INOUT_     bool    *hit_sel_addr1,
    _INOUT_     bool    *hit_sel_addr2,
    _INOUT_     bool    *hit_sel_addr3,
    _INOUT_     bool    *hit_sel_addr4,
    /** type = QUERY_CACHE   的输出 **/
    /** type = UPDATE_CACHE  的输入 **/
    _INOUT_     uint8_t     rd_wr_data0[DATA_WIDTH_B],
    _INOUT_     uint8_t     rd_wr_data1[DATA_WIDTH_B],
    _INOUT_     uint8_t     rd_wr_data2[DATA_WIDTH_B],
    _INOUT_     uint8_t     rd_wr_data3[DATA_WIDTH_B],
    _INOUT_     uint8_t     rd_wr_data4[DATA_WIDTH_B],

    /** Output singals **/
    _OUT_       bool        *ready,
    _OUT_       bool        *valid
)
{
#ifndef NO_BRAM
#pragma HLS RESOURCE variable=cache_map_tbl0 core=RAM_1P_BRAM
#pragma HLS RESOURCE variable=cache_map_tbl1 core=RAM_1P_BRAM
#pragma HLS RESOURCE variable=cache_map_tbl2 core=RAM_1P_BRAM
#pragma HLS RESOURCE variable=cache_map_tbl3 core=RAM_1P_BRAM
#else
#pragma HLS ARRAY_PARTITION variable=cache_map_tbl0 complete dim=2
#pragma HLS ARRAY_PARTITION variable=cache_map_tbl1 complete dim=2
#pragma HLS ARRAY_PARTITION variable=cache_map_tbl2 complete dim=2
#pragma HLS ARRAY_PARTITION variable=cache_map_tbl3 complete dim=2
#endif

#pragma HLS RESOURCE variable=cache_data_store0 core=RAM_1P_BRAM
#pragma HLS RESOURCE variable=cache_data_store1 core=RAM_1P_BRAM
#pragma HLS RESOURCE variable=cache_data_store2 core=RAM_1P_BRAM
#pragma HLS RESOURCE variable=cache_data_store3 core=RAM_1P_BRAM

    uint16_t coffset0, cgrpnum0;
    uint16_t coffset1, cgrpnum1;
    uint16_t coffset2, cgrpnum2;
    uint16_t coffset3, cgrpnum3;
    uint16_t coffset4, cgrpnum4;

    *valid = false;

    if (reset == true)
        reset_map_tbls();
    *ready = true;

    /************************ check address ************************/
    if (addr0 % DATA_WIDTH ||
        addr1 % DATA_WIDTH ||
        addr2 % DATA_WIDTH ||
        addr3 % DATA_WIDTH ||
        addr4 % DATA_WIDTH)
        return;

    /*********************** address convert ***********************/
    // 0
    coffset0 = addr0 % CAHCE0_NUM_BLOCKS;
    cgrpnum0 = addr0 / CAHCE0_NUM_BLOCKS;
    // 1 3
    coffset1 = addr1 % CAHCE1_NUM_BLOCKS;
    cgrpnum1 = addr1 / CAHCE1_NUM_BLOCKS;
    coffset3 = addr3 % CAHCE1_NUM_BLOCKS;
    cgrpnum3 = addr3 / CAHCE1_NUM_BLOCKS;
    // 2
    coffset2 = addr2 % CAHCE2_NUM_BLOCKS;
    cgrpnum2 = addr2 / CAHCE2_NUM_BLOCKS;
    // 4
    coffset4 = addr4 % CAHCE3_NUM_BLOCKS;
    cgrpnum4 = addr4 / CAHCE3_NUM_BLOCKS;

    /******************* query and copy from cache ******************/
    if (type == QUERY_CACHE)
    {
        //#pragma HLS DATAFLOW
        // cache 0 -> addr 0
        query_and_copy_block (coffset0, cgrpnum0, hit_sel_addr0, rd_wr_data0,
                              cache_map_tbl0, cache_data_store0);
        // cache 1 -> addr 1, addr 3
        query_and_copy_block (coffset1, cgrpnum1, hit_sel_addr1, rd_wr_data1,
                              cache_map_tbl1, cache_data_store1);
        query_and_copy_block (coffset3, cgrpnum3, hit_sel_addr3, rd_wr_data3,
                              cache_map_tbl1, cache_data_store1);
        // cache 2 -> addr 2
        query_and_copy_block (coffset2, cgrpnum2, hit_sel_addr2, rd_wr_data2,
                              cache_map_tbl2, cache_data_store2);
        // cache 3 -> addr 4
        query_and_copy_block (coffset4, cgrpnum4, hit_sel_addr4, rd_wr_data4,
                              cache_map_tbl3, cache_data_store3);
    }

    /********************** update cache data **********************/
    else // type == UPDATE_CACHE
    {
        //#pragma HLS DATAFLOW
        // cache 0 -> addr 0
        update_cache_data (coffset0, cgrpnum0, rd_wr_data0,
                           cache_map_tbl0, cache_data_store0);
        // cache 1 -> addr 1, addr 3
        update_cache_data (coffset1, cgrpnum1, rd_wr_data1,
                           cache_map_tbl1, cache_data_store1);
        update_cache_data (coffset3, cgrpnum3, rd_wr_data3,
                           cache_map_tbl1, cache_data_store1);
        // cache 2 -> addr 2
        update_cache_data (coffset2, cgrpnum2, rd_wr_data2,
                           cache_map_tbl2, cache_data_store2);
        // cache 3 -> addr 4
        update_cache_data (coffset4, cgrpnum4, rd_wr_data4,
                           cache_map_tbl3, cache_data_store3);
    }

    *valid = true;
}
