
// for main memory & cache
`define ADDR_WIDTH              (32)
`define DATA_WIDTH              (32)
// for cache
`define CAHCE_ADDR_WIDTH        (16)    // Have 2^CAHCE_ADDR_WIDTH Cache Blocks
`define CAHCE_NUM_BLOCKS        (2^`CAHCE_ADDR_WIDTH)
`define CAHCE_BLOCK_SIZEN       (4)
`define CAHCE_BLOCK_SIZE        (`DATA_WIDTH*`CAHCE_BLOCK_SIZEN)     // Cache Block Size in Bits

`define READ    0
`define WRITE   1

module Direct_Mapped_Cache (
    input                           clk,
    input                           rst,
    // upper request, addr aligned in cache block
    input      [`ADDR_WIDTH-1:0]    u_iaddr,
    inout      [`DATA_WIDTH-1:0]    u_data,
    input                           u_itype,
    input                           u_enable,
    output reg                      u_valid,
    // lower call, addr aligned in cache block
    output reg [`ADDR_WIDTH-1:0]    l_oaddr,
    inout      [`DATA_WIDTH-1:0]    l_data,
    output reg                      l_otype,
    output reg                      l_enable,
    input                           l_valid
);

// Direct Mapped Cache: just chop the system memory into the same number of chunks. Then each chunk gets the use of one cache line.
// Map Table Entry
`define MAP_TBL_ENTRY_SIZE          (`ADDR_WIDTH-`CAHCE_ADDR_WIDTH)
`define MAP_TBL_ENTRY_MASK          (2^`MAP_TBL_ENTRY_SIZE - 1)
// Direct Mapped Cache Table
reg [`MAP_TBL_ENTRY_SIZE-1:0]       cache_map_tbl [0:`CAHCE_NUM_BLOCKS-1];
// Table Offset & Table Entry
reg [`MAP_TBL_ENTRY_SIZE-1:0]       tbl_entry;
reg [`CAHCE_ADDR_WIDTH-1:0]         tbl_offset;

// Cache Storage Data
reg [`CAHCE_BLOCK_SIZE-1:0]         cache_data_store [0:`CAHCE_NUM_BLOCKS-1];
reg [`CAHCE_BLOCK_SIZE-1:0]         data_block;

reg [`DATA_WIDTH-1:0]               upper_data, lower_data;
assign u_data = upper_data;
assign l_data = lower_data;

reg [3:0] state;
reg [3:0] pushed_state;
`define IDLE                        4'b0000
`define ADDR_CVT                    4'b0001
`define CHK_CACHE                   4'b0010
`define CACHE_HIT_READ              4'b0011
`define CACHE_HIT_WRITE             4'b0100
`define CACHE_MISS_READ             4'b0101
`define CACHE_MISS_WRITE            4'b0110
`define CACHE_READ_UPDATE           4'b0111
`define CACHE_WRITE_UPDATE          4'b1000
`define OUTPUT_VALID                4'b1001
`define OUTPUT_DATA                 4'b1010

reg [3:0] data_idx;

always @ (posedge clk) begin
    if (rst == 1'b1) begin
        u_valid     <= 0;
        l_oaddr     <= 0;
        l_enable    <= 0;
    end else begin
        case (state)
            /*********************************************/
            `IDLE: begin
                u_valid <= 1'b0;
                pushed_state <= `IDLE;
                if (u_enable == 1'b1)
                    state <= `ADDR_CVT;
                else
                    state <= `IDLE;
            end
            /*********************************************/
            `ADDR_CVT: begin
                if (u_iaddr % `CAHCE_BLOCK_SIZE == 0) begin
                    tbl_entry  <= (u_iaddr >> `CAHCE_ADDR_WIDTH);
                    tbl_offset <= (u_iaddr  & `MAP_TBL_ENTRY_MASK);
                    state <= `CHK_CACHE;
                end else begin
                    state <= `IDLE;
                end
            end
            /*********************************************/
            `CHK_CACHE: begin
                data_idx <= 1;
                if (tbl_entry == cache_map_tbl[tbl_offset])
                    if (u_itype == `READ)
                        state <= `CACHE_HIT_READ;
                    else
                        state <= `CACHE_HIT_WRITE;
                else
                    if (u_itype == `READ)
                        state <= `CACHE_MISS_READ;
                    else
                        state <= `CACHE_MISS_WRITE;
            end
            /*********************************************/
            `CACHE_HIT_READ: begin
                // get data from cache pool
                data_block <= cache_data_store[tbl_offset];
                state <= `OUTPUT_DATA;
            end
            /*********************************************/
            `CACHE_HIT_WRITE: begin
                //TODO: get write data from upper
                //TODO: write to lower
                state <= `CACHE_WRITE_UPDATE;
                state <= `OUTPUT_VALID;
            end
            /*********************************************/
            `CACHE_MISS_WRITE: begin
                //TODO: get write data from upper
                //TODO: write to lower
                state <= `CACHE_WRITE_UPDATE;
                state <= `OUTPUT_VALID;
            end
            /*********************************************/
            `CACHE_MISS_READ: begin
                //TODO: get read data from lower
                l_oaddr <= u_iaddr;
                l_otype <= `READ;
                if (l_valid == 1) begin
                    upper_data = l_data[
                        ((`DATA_WIDTH*data_idx)-1) -:
                        `DATA_WIDTH
                    ];
                    data_idx <= data_idx + 1;
                end
                state <= `CACHE_READ_UPDATE;
                state <= `OUTPUT_DATA;
            end
            /*********************************************/
            `CACHE_READ_UPDATE: begin
                cache_map_tbl[tbl_offset] <= tbl_entry;
                cache_data_store[tbl_offset] <= data_block;
                state <= pushed_state;
            end
            /*********************************************/
            `CACHE_WRITE_UPDATE: begin
                cache_map_tbl[tbl_offset] <= tbl_entry;
                cache_data_store[tbl_offset] <= data_block;
                state <= pushed_state;
            end
            /*********************************************/
            `OUTPUT_DATA: begin
                if (data_idx <= `CAHCE_BLOCK_SIZEN) begin
                    u_valid = 1;
                    upper_data = data_block[
                        ((`DATA_WIDTH*data_idx)-1) -:
                        `DATA_WIDTH
                    ];
                    data_idx = data_idx + 1;
                    if (data_idx == `CAHCE_BLOCK_SIZEN) begin
                        data_idx <= 0;
                        state <= `IDLE;
                    end
                end
            end
            /*********************************************/
            `OUTPUT_VALID: begin
                u_valid <= 1;
            end
            /*********************************************/
            default:
                state <= `IDLE;
        endcase
    end
end

endmodule
