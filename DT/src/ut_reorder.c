#include "incs.h"
#include "preproc_j.h"
#include "check.h"
#include "reorder.h"
#include "reorder_isp.h"
#include "get_time_usec.h"
#include "qsort_asc_val_F4_idx_I1.c"
#ifdef SEQUENTIAL
int g_num_swaps;
#endif
config_t g_C;

typedef struct _composite_t { 
  float val;
  uint8_t goal;
} composite_t;

static int sortfn(
    const void *X, 
    const void *Y
    )
{
  const composite_t *a = (const composite_t *)X;
  const composite_t *b = (const composite_t *)Y;
  if ( a->val < b->val ) {
    return 1;
  }
  else {
    return 0;
  }
}

int
main(
    int argc,
    char **argv
    )
{
  int status = 0;
  float *X = NULL; // for re-sort timing comparison
  composite_t *tmpXg = NULL; // for re-sort timing comparison
  uint8_t *g = NULL; // for re-sort timing comparison

  uint64_t *Y    = NULL;
  uint64_t *tmpY = NULL;
  uint64_t *isp_tmpY = NULL;

  uint32_t *yval = NULL;
  uint8_t  *goal = NULL;
  uint32_t *from = NULL;
  
  uint32_t *pre_yval = NULL;
  uint8_t  *pre_goal = NULL;
  uint32_t *pre_from = NULL;
  
  uint32_t *post_yval = NULL;
  uint8_t  *post_goal = NULL;
  uint32_t *post_from = NULL;
  
  uint32_t *to   = NULL;
  uint32_t *isp_to   = NULL;

  uint32_t *to_split = NULL;

  uint32_t n = 65536; // number of elements
  uint32_t lb = 0; 
  uint64_t t_start, t_stop, t_reorder, t_isp_reorder, t_resort;

#ifdef SEQUENTIAL
  g_num_swaps = 0;
#endif
  if ( argc >= 2 ) { 
    n = atoi(argv[1]);
  }
  if ( n <= 0 ) { go_BYE(-1); } 
  printf("n = %d \n", n);
  uint32_t ub = n;
  //-----------------------------------------
  Y    = malloc(n * sizeof(uint64_t));
  tmpY = malloc(n * sizeof(uint64_t));
  isp_tmpY = malloc(n * sizeof(uint64_t));

  yval = malloc(n * sizeof(uint32_t));
  from = malloc(n * sizeof(uint32_t));
  goal = malloc(n * sizeof(uint8_t));

  pre_yval = malloc(n * sizeof(uint32_t));
  pre_from = malloc(n * sizeof(uint32_t));
  pre_goal = malloc(n * sizeof(uint8_t));

  post_yval = malloc(n * sizeof(uint32_t));
  post_from = malloc(n * sizeof(uint32_t));
  post_goal = malloc(n * sizeof(uint8_t));

  to   = malloc(n * sizeof(uint32_t));
  isp_to   = malloc(n * sizeof(uint32_t));

  to_split   = malloc(n * sizeof(uint32_t));
  // Initialization
  for ( uint32_t i = 0; i < n; i++ ) { yval[i] = i+1; }
  for ( uint32_t i = 0; i < n; i++ ) { from[i] = (n-1) - i; }
  for ( uint32_t i = 0; i < n; i++ ) { goal[i] = i % 2 ; } 
  for ( uint32_t i = 0; i < n; i++ ) { 
    Y[i] = x_mk_comp_val(from[i], goal[i], yval[i]); 
  }
  for ( uint32_t i = 0; i < n; i++ ) { tmpY[i] = 0; }
  // We decree that half the points go left, and other half go right
  uint32_t lidx = 0;
  uint32_t ridx = n / 2;
  uint32_t split_yidx = n / 2;
  uint32_t p1 = 0, p2 = n - 1;
  for ( uint32_t i = 0; i < n; ) { 
    to_split[i++] = p1++;
    to_split[i++] = p2--;
  }
  //-----------------------------------------
  t_start = get_time_usec();
  status = reorder(Y, tmpY, to, to_split, lb, ub, split_yidx, &lidx, &ridx);
  cBYE(status);
  t_stop = get_time_usec();
  t_reorder = t_stop - t_start;
  for ( uint32_t i = 0; i < n; i++ ) { 
    pre_from[i] = get_from(Y[i]);
    pre_goal[i] = get_goal(Y[i]);
    pre_yval[i] = get_yval(Y[i]);
    post_from[i] = get_from(tmpY[i]);
    post_goal[i] = get_goal(tmpY[i]);
    post_yval[i] = get_yval(tmpY[i]);
    /*
    fprintf(stdout, "%d,%d,%d||%d,%d,%d\n", 
      pre_from[i], pre_goal[i], pre_yval[i], 
      post_from[i], post_goal[i], post_yval[i]);
      */
  }
  bool is_eq;
  status = chk_set_equality(pre_from, post_from, n, &is_eq); cBYE(status);
  if ( !is_eq ) { go_BYE(-1); }
  status = chk_set_equality(pre_yval, post_yval, n, &is_eq); cBYE(status);
  if ( !is_eq ) { go_BYE(-1); }
  int cnt1 = 0, cnt2 = 0;
  for ( uint32_t i = 0; i < n; i++ ) { 
    cnt1 += pre_goal[i];
    cnt2 += pre_goal[i];
  }
  if ( cnt1 != cnt2 ) { go_BYE(-1); }
  if ( lidx != n / 2 ) { go_BYE(-1); }
  if ( ridx != n     ) { go_BYE(-1); }
  //--- run ISP version
  lidx = 0;
  ridx = n / 2;
  t_start = get_time_usec();
  reorder_isp(Y, isp_tmpY, isp_to, to_split, lb, ub, split_yidx, 
      &lidx, &ridx, &status);
  cBYE(status);
  t_stop = get_time_usec();
  t_isp_reorder = t_stop - t_start;
  // --- compare ISP results with C results 
  if ( lidx != n / 2 ) { go_BYE(-1); }
  if ( ridx != n     ) { go_BYE(-1); }
  for ( uint32_t i = 0; i < n; i++ ) { 
    if ( tmpY[i] != isp_tmpY[i] ) { go_BYE(-1); }
    if ( to[i] != isp_to[i] ) { go_BYE(-1); }
  }
  // Now we do a timing comparison with a re-sort
  // Note that this is not a correctness test
  // We want to calculate the time to re-sort given that 
  // every other point goes left (same assumption as for reorder)

  X = malloc(n * sizeof(float));
  return_if_malloc_failed(X);
  tmpXg = malloc(n * sizeof(composite_t));
  return_if_malloc_failed(tmpXg);
  g = malloc(n * sizeof(uint8_t));
  return_if_malloc_failed(g);
  // set some random values for X
  for ( uint32_t i = 0; i < n; i++ ) { 
    X[i] = random();
    g[i] = random() & 0x1; // randomly set to 0 or 1 
  }
  t_start = get_time_usec();
  // Using a buffer, tmpXg
  // move the left points to one side and the right points to the other
  lidx = 0;
  ridx = n/2;
  bool is_left = true;
  for ( uint32_t i = 0; i < n; i++ ) { 
    if ( is_left ) { 
      tmpXg[lidx].val  = X[i];
      tmpXg[lidx].goal = g[i];
      lidx++;
      is_left = false;
    }
    else {
      tmpXg[ridx].val  = X[i];
      tmpXg[ridx].goal = g[i];
      ridx++;
      is_left = true;
    }
  }
  // sort the buffer
  qsort (tmpXg, n, sizeof(composite_t), sortfn);
  // move the points back from the buffer
  for ( uint32_t i = 0; i < n; i++ ) { 
    X[i] = tmpXg[i].val;
    g[i] = tmpXg[i].goal;
  }
  //---------------------
  t_stop = get_time_usec();
  t_resort = t_stop - t_start;

  
  fprintf(stderr, "Time reorder     = %" PRIu64 "\n", t_reorder);
  fprintf(stderr, "Time reorder isp = %" PRIu64 "\n", t_isp_reorder);
  fprintf(stderr, "Time resort      = %" PRIu64 "\n", t_resort);
  fprintf(stderr, "Completed test [%s] successfully\n", argv[0]);
BYE:
  free_if_non_null(X);
  free_if_non_null(g);
  free_if_non_null(tmpXg);

  // Free stuff so that we have space for re-sort test 
  free_if_non_null(Y);
  free_if_non_null(tmpY);
  free_if_non_null(isp_tmpY);

  free_if_non_null(yval);
  free_if_non_null(goal);
  free_if_non_null(from);
  
  free_if_non_null(pre_yval);
  free_if_non_null(pre_goal);
  free_if_non_null(pre_from);
  
  free_if_non_null(post_yval);
  free_if_non_null(post_goal);
  free_if_non_null(post_from);
  
  free_if_non_null(to);
  free_if_non_null(isp_to);

  free_if_non_null(to_split);
  //-------------------------------
  return status;
}
