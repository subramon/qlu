
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include "q_macros.h"
#include <math.h>


extern int
Output_I4(
       int32_t **src_bufs,      
       int *ptr_weight, 
       int32_t *last_packet,
       int last_packet_incomplete,
       uint64_t last_packet_siz, 
       uint64_t eff_siz, 
       uint64_t num_quantiles,
       int32_t *ptr_y,
       uint32_t b,         
       uint32_t k
       );

  
