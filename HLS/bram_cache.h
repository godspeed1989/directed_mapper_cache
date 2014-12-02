#ifndef BRAM_CACHE_H
#define BRAM_CACHE_H

#include <inttypes.h>

#define _IN_
#define _OUT_
#define _INOUT_

#define DATA_WIDTH 			256
#define DATA_WIDTH_B		32   /* 256/8 */

#define CAHCE0_NUM_BLOCKS	1024
#define CAHCE1_NUM_BLOCKS	512
#define CAHCE2_NUM_BLOCKS	4096
#define CAHCE3_NUM_BLOCKS	2048

#define _N_					4

#define QUERY_CACHE			0
#define UPDATE_CACHE		1

// Cache映射表内的元素大小
//   ?      cnt    non-empty  grpnum
// [ 1 ]   [ 2 ]     [1]       [16]

typedef struct map_tbl_t {
	uint8_t		ref_cnt;	// reference count
	uint8_t		is_used;	// used or not
	uint16_t	grpnum;		// group number
}map_tbl_t;

#endif
