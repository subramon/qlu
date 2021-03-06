local Q       = require 'Q/q_export'
local lVector = require 'Q/RUNTIME/VCTR/lua/lVector'
local Scalar  = require 'libsclr'
local cmem    = require 'libcmem'
local qconsts = require 'Q/UTILS/lua/qconsts'
local to_scalar = require 'Q/UTILS/lua/to_scalar'
local rev_lkp =  require 'Q/UTILS/lua/rev_lkp'

local good_qtypes = rev_lkp({ "I1",  "I2",  "I4", "I8",  "F4", "F8", "B1", "SC"})

local mk_col = function (
  input, 
  qtype, 
  nn_input
  )
  local doc_string = [[ Signature: Q.mk_col(input, qtype, opt_nn_input)
-- creates a column of input table values of input qtype
1) input: array of values
2) qtype: desired qtype of column
3) nn_input: array of nn values
-- returns: column of input values of qtype"
]]
  -- this call has been just done for docstring
  if input and input == "help" then
      return doc_string
  end

  assert(type(input) == "table")
  assert(#input > 0, "Input table has no entries")
  local has_nulls = false
  if ( nn_input ) then 
    assert(type(nn_input) == "table")
    assert(#nn_input == #input)
    has_nulls = true
  end
  
  assert(good_qtypes[qtype], qtype)
  local width
  local ctype =  assert(qconsts.qtypes[qtype].ctype)
  local table_length = table.getn(input)
  local length_in_bytes = nil
  local chunk = nil
  
  local col
  if ( qtype == "SC" ) then 
    assert(not nn_input) -- no nulls for SC
    --== Figure out width
    width = 0
    for k, v in pairs(input) do 
      assert(type(v) == "string")
      if ( #v > width ) then width = #v end
    end
    width = width + 1 -- add space for nullc
    --=====================
    col = lVector({ qtype=qtype, width = width, has_nulls = false})
    for k, v in pairs(input) do 
      local sval = cmem.new( { size = width, qtype = "SC"})
      assert(sval:set(v))
      col:put1(sval)
    end
  else
    col = lVector({ qtype=qtype, has_nulls = has_nulls})
    for k, v in ipairs(input) do
      local v_nn = nil
      v = assert(to_scalar(v, qtype))
       if ( nn_input ) then 
         local v = nn_input[k]
         assert(type(v) == "boolean")
         v_nn = Scalar.new(v, "B1")
       end
      col:put1(v, v_nn)
    end
  end
  col:eov()
  return col
end
return require('Q/q_export').export('mk_col', mk_col)
