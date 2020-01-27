local ffi       = require 'ffi'
local cmem      = require 'libcmem'
local cVector   = require 'libvctr'
local Scalar    = require 'libsclr'
local to_scalar = require 'Q/UTILS/lua/to_scalar'
local is_in     = require 'Q/UTILS/lua/is_in'
local get_ptr   = require 'Q/UTILS/lua/get_ptr'
local qconsts   = require 'Q/UTILS/lua/q_consts'
local tmpl      = qconsts.Q_SRC_ROOT .. "/OPERATORS/S_TO_F/lua/period.tmpl"

return function (
  in_args
  )
  --====================================
  assert(type(in_args) == "table")
  local qtype = assert(in_args.qtype)
  local len   = assert(in_args.len)
  local start = assert(in_args.start)
  local by    = assert(in_args.by)
  local period= assert(in_args.period)
  local ctype = assert(qconsts.qtypes[qtype].ctype)
  assert(is_in(qtype, { "I1", "I2", "I4", "I8", "F4", "F8"}))
  assert(len > 0, "vector length must be positive")
  assert(period > 0, "period must be positive")

  local subs = {};
  --========================
  subs.fn	    = "period_" .. qtype
  subs.len	    = len
  subs.out_qtype    = qtype
  subs.out_ctype    = qconsts.qtypes[qtype].ctype
  subs.tmpl         = tmpl
  subs.buf_size = cVector.chunk_size() * qconsts.qtypes[qtype].width
  --========================
  -- set up args for C code
  local sstart = assert(to_scalar(start, qtype))
  sstart = ffi.cast("SCLR_REC_TYPE *", sstart)
  local sby    = assert(to_scalar(by, qtype))
  sby    = ffi.cast("SCLR_REC_TYPE *", sby)
  local speriod    = assert(to_scalar(period, "I4"))
  speriod    = ffi.cast("SCLR_REC_TYPE *", speriod)

  local args_ctype = "PERIOD_" .. qtype .. "_REC_TYPE";
  local sz = ffi.sizeof(args_ctype)
  local cargs = cmem.new(sz, qtype, qtype); 
  local args = ffi.cast(args_ctype .. " *", get_ptr(cargs))

  args[0]["start"]  =  sstart[0].cdata["val" .. qtype]
  args[0]["by"]     =     sby[0].cdata["val" .. qtype]
  args[0]["period"] = speriod[0].cdata["valI4"]

  subs.args       = args
  subs.args_ctype = args_ctype
  return subs
end
