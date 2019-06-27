#include "q_incs.h"
#include "mk_loc.h"
#include "fastdiv.h"
//------------------------------------------------------
int 
mk_loc(
    uint32_t *hashes, // input  [nkeys] 
    uint32_t nkeys, // input 
    uint32_t hmap_size, // input 
    uint32_t *locs // output [nkeys] 
    )
{
  int status = 0;
  int chunk_size = 1024;
  uint64_t divinfo = fast_div32_init(hmap_size);
#pragma omp parallel for schedule(static, chunk_size)
  for ( uint32_t i = 0; i < nkeys; i++ ) {
    locs[i] = fast_rem32(hashes[i], hmap_size, divinfo);
  }
  return status;
}
//------------------------------------------------------