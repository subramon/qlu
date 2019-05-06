
#include "q_incs.h"
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct _reduce_sum_sqr_I4_args {
  uint64_t sum_sqr_val;
  uint64_t num; // number of non-null elements inspected
  } REDUCE_sum_sqr_I4_ARGS;
  
extern int
sum_sqr_I4(  
      const int32_t * restrict in,  
      uint64_t nR,
      void *ptr_args,
      uint64_t idx
      ) 
;

   
