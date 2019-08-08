local function write_to_file(content, fname)
  local file = assertx(io.open(fname, "w+"), "unable to create ", fname)
  -- local str = content:gsub('\n', [[\n]])
  file:write(content)
  file:close()
end
return write_to_file
