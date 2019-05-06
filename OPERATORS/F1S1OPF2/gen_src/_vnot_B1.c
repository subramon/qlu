
#include "_vnot_B1.h"

int
vnot_B1(  
      const uint64_t * const in,
      uint64_t *nn_in,
      uint64_t nR,
      void *dummy,
      uint64_t * restrict out,  
      uint64_t *nn_out
      )

{
  int status = 0;
  // Using ceil operator because this will include the last 64 bit chunk for the processing 
  // though there might be some unused bits in last 64 bit chunk but vector code is taking care of it (not using those bits).
  uint64_t loop_count = ceil((double)nR / 64);
  if ( in == NULL ) { go_BYE(-1); }
  if ( nR == 0 ) { go_BYE(-1); }

#pragma omp parallel for schedule(static)
  for ( uint64_t i = 0; i < loop_count; i++ ) {
    //TODO: Do not modify unused bits from last 64 bits
    out[i] = ~ in[i];
  }
BYE:
  if ( status < 0 ) { WHEREAMI; }
  return status;
}
   
