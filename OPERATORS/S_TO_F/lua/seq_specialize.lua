local ffi       = require 'ffi'
local cmem      = require 'libcmem'
local cVector   = require 'libvctr'
local Scalar    = require 'libsclr'
local to_scalar = require 'Q/UTILS/lua/to_scalar'
local is_in     = require 'Q/UTILS/lua/is_in'
local get_ptr   = require 'Q/UTILS/lua/get_ptr'
local qconsts   = require 'Q/UTILS/lua/qconsts'
local qc        = require 'Q/UTILS/lua/qcore'
local qmem      = require 'Q/UTILS/lua/qmem'
local chunk_size = qmem.chunk_size

-- cdef the necessary struct within pcall to prevent error on second call
local incs = { "RUNTIME/CMEM/inc/", "UTILS/inc/" }
qc.q_cdef("OPERATORS/S_TO_F/inc/seq_struct.h", incs)
qc.q_cdef("RUNTIME/SCLR/inc/sclr_struct.h", incs)
return function (
  largs
  )
  --====================================
  assert(type(largs) == "table")
  local start = assert(largs.start)
  local qtype = assert(largs.qtype)
  local len   = assert(largs.len)
  local by    = assert(largs.by)
  local ctype = assert(qconsts.qtypes[qtype].ctype)
  assert(is_in(qtype, { "I1", "I2", "I4", "I8", "F4", "F8"}))
  assert(len > 0, "vector length must be positive")

  local subs = {};
  --========================
  subs.fn	    = "seq_" .. qtype
  subs.len	    = len
  subs.out_qtype    = qtype
  subs.out_ctype    = qconsts.qtypes[qtype].ctype
  subs.out_buf_size = chunk_size * qconsts.qtypes[qtype].width
  subs.cst_out_as = subs.out_ctype .. " * "
  --========================
  -- set up args for C code
  local sstart = assert(to_scalar(start, qtype))
  local sby    = assert(to_scalar(by, qtype))

  -- allocate cargs
  subs.cargs_ctype = "SEQ_" .. qtype .. "_REC_TYPE";
  local sz = ffi.sizeof(subs.cargs_ctype)
  subs.cargs = cmem.new(sz)
  subs.cargs:zero()
  subs.cst_cargs_as = subs.cargs_ctype .. " *"

  -- initialize cargs from scalar values 
  local cargs = get_ptr(subs.cargs, subs.cst_cargs_as)

  sstart = ffi.cast("SCLR_REC_TYPE *", sstart)
  sby    = ffi.cast("SCLR_REC_TYPE *", sby)

  cargs[0]["start"] = sstart[0].cdata["val" .. qtype]
  cargs[0]["by"]    =    sby[0].cdata["val" .. qtype]

  subs.tmpl   = "OPERATORS/S_TO_F/lua/seq.tmpl"
  subs.incdir = "OPERATORS/S_TO_F/gen_inc/"
  subs.srcdir = "OPERATORS/S_TO_F/gen_src/"
  subs.incs   = { "UTILS/inc", "OPERATORS/S_TO_F/inc/", "OPERATORS/S_TO_F/gen_inc/", }
  subs.structs = { "OPERATORS/S_TO_F/inc/seq_struct.h" }
  return subs
end
