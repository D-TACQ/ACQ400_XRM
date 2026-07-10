-- FNAL Multicast Table (FMT) dissector
-- Expected payload:
--  64 rows x 16 bytes each = 1024 bytes total
--  struct FMT_ROW {
--      uint16 event;
--      uint16 pad;
--      uint32 client_data;
--      int64  timestamp;
--  };

local fmt = Proto("fmt", "FNAL Multicast Table")

local f_row        = ProtoField.uint8("fmt.row", "Row Index", base.DEC)
local f_event      = ProtoField.uint16("fmt.event", "Event", base.DEC)
local f_pad        = ProtoField.uint16("fmt.pad", "Pad", base.HEX)
local f_clientdata = ProtoField.uint32("fmt.client_data", "Client Data", base.HEX)
local f_timestamp  = ProtoField.int64("fmt.timestamp", "Timestamp (usec since EPOCH)", base.DEC)

fmt.fields = {
    f_row,
    f_event,
    f_pad,
    f_clientdata,
    f_timestamp
}

local ROWS = 64
local ROW_SIZE = 16
local EXPECTED_LEN = ROWS * ROW_SIZE

function fmt.dissector(buf, pinfo, tree)
    pinfo.cols.protocol = "FMT"

    local len = buf:len()
    local root = tree:add(fmt, buf())

    if len ~= EXPECTED_LEN then
        root:add_expert_info(expert.group.MALFORMED, expert.severity.ERROR,
            string.format("Expected %d bytes (%d rows x %d bytes), got %d",
                          EXPECTED_LEN, ROWS, ROW_SIZE, len))
        pinfo.cols.info:set(string.format("FMT malformed len=%d", len))
        return len
    end

    pinfo.cols.info:set(string.format("FMT %d rows", ROWS))

    local offset = 0
    for i = 0, ROWS - 1 do
        local row_tvb = buf(offset, ROW_SIZE)
        local row = root:add(fmt, row_tvb, string.format("Row %d", i))

        row:add(f_row, buf(offset, 0), i)

        row:add(f_event,     buf(offset,   2))
        row:add(f_pad,       buf(offset+2, 2))
        row:add(f_clientdata,buf(offset+4, 4))
        row:add(f_timestamp, buf(offset+8, 8))

        offset = offset + ROW_SIZE
    end

    return len
end

-- Register to a UDP port for multicast traffic.
-- FMT default port 5055
local udp_port = DissectorTable.get("udp.port")
udp_port:add(5055, fmt)


