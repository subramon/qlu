local function rev_lkp(X)
  assert(type(X) == "table")
  local vals = {}
  for k, v in pairs(X) do 
    vals[v] = true
  end
  return vals
end
return rev_lkp