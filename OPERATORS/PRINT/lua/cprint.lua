local cutils   = require 'libcutils'
local cmem     = require 'libcmem'
local ffi      = require 'ffi'
local qc       = require 'Q/UTILS/lua/qcore'
local qconsts  = require 'Q/UTILS/lua/qconsts'
local get_ptr  = require 'Q/UTILS/lua/get_ptr'
local qmem    = require 'Q/UTILS/lua/qmem'
local chunk_size = qmem.chunk_size

local function cprint( 
  opfile, -- file for destination fo print
  where, -- nil or lVector of type B1
  lb, -- number
  ub, -- number
  V -- table of lVectors to be printed
  )
  local func_name = "cprint"
  local subs = {}
  subs.fn = func_name
  -- IMPORTANT: In specifying files, Do not start with a backslash
  subs.dotc = "OPERATORS/PRINT/src/cprint.c"
  subs.doth = "OPERATORS/PRINT/inc/cprint.h"
  subs.srcs = { "UTILS/src/get_bit_u64.c" }
  subs.incs = { "OPERATORS/PRINT/inc/", "UTILS/inc/" }
  subs.structs = nil -- no structs need to be cdef'd
  subs.libs = nil -- no libaries need to be linked
  qc.q_add(subs); 

  if ( opfile ) then 
    cutils.delete(opfile) -- clean out any existing file
  else
    -- printing to stdout 
  end
  local function min(x, y) if x < y then return x else return y end end
  local function max(x, y) if x > y then return x else return y end end
  local nC = #V -- determine number of columns to be printed
  local chunk_size = chunk_size
  local chunk_num = math.floor(lb / chunk_size) -- first usable chunk
  -- C = pointers to data to be printed
  local sz = nC * ffi.sizeof("void *")
  local orig_C = assert(cmem.new(sz))
  orig_C:zero()
  local C = get_ptr(orig_C, "void **")
  -- F array of fldtypes 
  local F = assert(cmem.new({size = (nC * ffi.sizeof("int")), name = "F"}))
  F:zero()
  F = get_ptr(F, "I4")
  -- W array of widths 
  local W = assert(cmem.new({size = nC * ffi.sizeof("int"), name = "W"}))
  W:zero()
  W = get_ptr(W, "I4")
  -- START: Assemble F and W
  for i, v in ipairs(V) do 
    local qtype = v:fldtype()
    qtype = qconsts.qtypes[qtype].cenum -- convert string to integer for C
    local width = v:width()
    -- Note: we create temporary local variables because the
    -- function calls return 2 things not just a single number
    assert(( i >= 1 ) and ( i <= nC ))
    F[i-1] = qtype
    W[i-1] = width
  end
  local c_opfile = ffi.NULL
  if ( opfile ) then 
    c_opfile = cmem.new({ size = #opfile+1, qtype = "SC", name='fname'})
    c_opfile:set(opfile)
    c_opfile = get_ptr(c_opfile, "char *")
  end
  --======================
  while true do 
    local clb = chunk_num * chunk_size
    local cub = clb + chunk_size
    if ( clb >= ub ) then break end -- TODO verify boundary conditions
    if ( cub <  lb ) then break end -- TODO verify boundary conditions
    -- [xlb, xub) is what we print from this chunk
    local xlb = max(lb, clb) 
    local xub = min(ub, cub)
    local wlen -- length of where fld if any 
    local chk_len -- to make sure all get_chunk() calls return same length 
    local cfld -- pointer to where fld or nil
    if ( where ) then 
      local wchunk
      wlen, wchunk = where:get_chunk(chunk_num)
      assert(wlen > 0)
      cfld = get_ptr(wchunk, "uint64_t *")
    end
    for i, v in ipairs(V) do
      local len, chnk = v:get_chunk(chunk_num)
      if ( not chk_len ) then chk_len = len else assert(chk_len == len) end 
      assert(len > 0)
      C[i-1] = get_ptr(chnk, "void *")
    end
    if ( wlen ) then assert( chk_len == wlen) end 
    local status = qc[func_name](c_opfile, cfld, C, nC, xlb - clb, 
      xub - xlb, F, W)
    assert(status == 0)
    -- release chunks 
    if ( where ) then 
      where:unget_chunk(chunk_num)
    end
    for i, v in ipairs(V) do
      v:unget_chunk(chunk_num)
    end
    chunk_num = chunk_num + 1 
  end
  orig_C:delete()
  return true
end
return cprint
