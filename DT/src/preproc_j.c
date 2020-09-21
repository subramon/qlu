#include "incs.h"
#include "preproc_j.h"

typedef struct _comp_key_t { 
  float xval;
  uint32_t idx;
  uint8_t g;
} comp_key_t;

static int
sortfn(
    const void *p1, 
    const void *p2
    )
{
  const comp_key_t *r1 = (const comp_key_t *)p1;
  const comp_key_t *r2 = (const comp_key_t *)p2;
  if ( r1->xval < r2->xval ) { 
    return -1;
  }
  else  {
    return 1;
  }
}
// #define mk_comp_val(x, y, z ) { ( x << 32 ) | ( z < 31 )  | y }
// #define get_from(x) { x >> 32 } 
// #define get_goal(x) { ( x >> 31 ) & 0x1 } 

int 
preproc_j(
    float *Xj, /* [m][n] */
    uint32_t n,
    uint8_t *g,
    uint64_t **ptr_Yj,
    uint32_t **ptr_to
   )
{
  int status = 0;
  uint64_t *Yj = NULL;
  uint32_t *to = NULL;
  comp_key_t *C = NULL;
  // allocate Y and idx 
  Yj  = malloc(n * sizeof(uint64_t));
  to  = malloc(n * sizeof(uint32_t));
  C   = malloc(n * sizeof(comp_key_t));
  for ( uint32_t i = 0; i < n; i++ ) { 
    C[i].idx  = i;
    C[i].g    = g[i];
    C[i].xval = Xj[i];
  }
  // sort X, idx, g
  qsort(C, n, sizeof(comp_key_t), sortfn);
  // create Y 
  uint32_t i = 0;
  float xval = C[i].xval;
  uint32_t yval = 1;
  uint32_t from_i = C[i].idx;
  Yj[i] = mk_comp_val(from_i, g[i], yval); 
  //-------------------------------------------
  uint32_t chk_yval = get_yval(Yj[i]);
  if ( chk_yval != yval ) { go_BYE(-1); }
  uint8_t  chk_goal = get_goal(Yj[i]);
  if ( chk_goal != g[i] ) { go_BYE(-1); }
  uint32_t chk_from = get_from(Yj[i]);
  if ( chk_from != from_i ) { go_BYE(-1); }
  //-------------------------------------------

  for ( i = 1; i < n; i++ ) { 
    if ( C[i].xval != xval ) {
      xval = C[i].xval;
      yval++;
    }
    from_i = C[i].idx;
    Yj[i] = mk_comp_val(from_i, yval, g[i]);
  }
  //--------------------------------------
  for ( i = 0; i < n; i++ ) { 
    uint32_t pos = get_from(Yj[i]);
    if ( pos >= n ) { go_BYE(-1); }
    to[pos] = i;
  }
  *ptr_Yj = Yj;
  *ptr_to = to;
BYE:
  free_if_non_null(C);
  if ( status < 0 ) { 
    free_if_non_null(Yj);
    free_if_non_null(to);
  }
  return status;
}
