
#include "q_incs.h"
#include "_set_bit_u64.h"
#include "_bin_search_I1.h"
extern int
bin_search_ainb_I2_I1(  
      const int16_t * restrict a,  
      uint64_t nA,
      const int8_t * restrict b,  
      uint32_t nB,
      uint64_t *C // output  nA bytes
      );
   
