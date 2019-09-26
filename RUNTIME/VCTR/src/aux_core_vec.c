#include "q_incs.h"
#include "core_vec_struct.h"

#include "aux_core_vec.h"
#include "_get_file_size.h"
#include "_isfile.h"
#include "_rdtsc.h"
#include "_rs_mmap.h"
#include "vec_globals.h"

#define NUM_CHUNKS_TO_ALLOCATE 65536

void 
l_memcpy(
    void *dest,
    const void *src,
    size_t n
    )
{
  uint64_t delta = 0, t_start = RDTSC(); n_memcpy++;
  memcpy(dest, src, n);
  delta = RDTSC() - t_start; if ( delta > 0 ) { t_memcpy += delta; }
}

void 
l_memset(
    void *s, 
    int c, 
    size_t n
    )
{
  uint64_t delta = 0, t_start = RDTSC(); n_memset++;
  memset(s, c, n);
  delta = RDTSC() - t_start; if ( delta > 0 ) { t_memset += delta; }
}

void *
l_malloc(
    size_t n
    )
{
  int status = 0;
  void  *x = NULL;
  uint64_t delta = 0, t_start = RDTSC(); n_malloc++;

  status = posix_memalign(&x, ALIGNMENT, n); 
  if ( status < 0 ) { WHEREAMI; return NULL; }
  if ( x == NULL ) { WHEREAMI; return NULL; }
  if ( status < 0 ) { WHEREAMI; return NULL; }
  delta = RDTSC() - t_start; if ( delta > 0 ) { t_malloc += delta; }

  return x;
}

bool 
is_file_size_okay(
    const char *const file_name,
    int64_t expected_size
    )
{
  if ( ( *file_name == '\0' ) && ( expected_size == 0 ) ) { return true; }
  int64_t actual_size = get_file_size(file_name);
  if ( actual_size !=  expected_size ) { return false; }
  return true;
}

int 
chk_name(
    const char * const name
    )
{
  int status = 0;
  if ( name == NULL ) { go_BYE(-1); }
  if ( strlen(name) > Q_MAX_LEN_INTERNAL_NAME ) {go_BYE(-1); }
  for ( char *cptr = (char *)name; *cptr != '\0'; cptr++ ) { 
    if ( !isascii(*cptr) ) { 
      fprintf(stderr, "Cannot have character [%c] in name \n", *cptr);
      go_BYE(-1); 
    }
    if ( ( *cptr == ',' ) || ( *cptr == '"' ) || ( *cptr == '\\') ) {
      go_BYE(-1);
    }
  }
BYE:
  return status;
}

int
chk_field_type(
    const char * const field_type,
    uint32_t field_width
    )
{
  int status = 0;
  if ( field_type == NULL ) { go_BYE(-1); }
  // TODO P3: SYNC with qtypes in q_consts.lua
  if ( ( strcmp(field_type, "B1") == 0 ) || 
       ( strcmp(field_type, "I1") == 0 ) || 
       ( strcmp(field_type, "I2") == 0 ) || 
       ( strcmp(field_type, "I4") == 0 ) || 
       ( strcmp(field_type, "I8") == 0 ) || 
       ( strcmp(field_type, "F4") == 0 ) || 
       ( strcmp(field_type, "F8") == 0 ) || 
       ( strcmp(field_type, "SC") == 0 ) || 
       ( strcmp(field_type, "TM") == 0 ) ) {
    /* all is well */
  }
  else {
    fprintf(stderr, "Bad field type = [%s] \n", field_type);
    go_BYE(-1);
  }
  if ( strcmp(field_type, "B1") == 0 )  {
    if ( field_width != 1 ) { go_BYE(-1); }
  }
  else {
    if ( field_width == 0 ) { go_BYE(-1); }
  }
  if ( strcmp(field_type, "SC") == 0 )  {
    if ( field_width < 2 ) { go_BYE(-1); }
  }
BYE:
  return status;
}

int
free_chunk(
    uint32_t chunk_dir_idx,
    bool is_persist
    )
{
  int status = 0;
  status = chk_chunk(chunk_dir_idx); cBYE(status);

  CHUNK_REC_TYPE *ptr_chunk =  g_chunk_dir+chunk_dir_idx;
  free_if_non_null(ptr_chunk->data);
  if ( !is_persist ) { 
    char file_name[Q_MAX_LEN_FILE_NAME+1];
    status = mk_file_name(ptr_chunk->uqid, file_name); cBYE(status);
    if ( isfile(file_name) ) {
      remove(file_name);
    }
  }
  memset(ptr_chunk, '\0', sizeof(CHUNK_REC_TYPE));
BYE:
  return status;
}


int
load_chunk(
    CHUNK_REC_TYPE *ptr_chunk, 
    uint32_t chunk_num,
    VEC_REC_TYPE *ptr_vec
    )
{
  int status = 0;
  if ( ptr_chunk->data != NULL ) { return status; } // already loaded
  //-- Get the chunk from its backup file if it exists
  if ( ptr_chunk->is_file ) {
    char file_name[Q_MAX_LEN_FILE_NAME+1];
    status = mk_file_name(ptr_chunk->chunk_num, file_name); cBYE(status);
    char *X = NULL; size_t nX = 0;
    status = rs_mmap(file_name, &X, &nX, 0); cBYE(status);
    if ( X == NULL ) { go_BYE(-1); }
    if ( nX != ptr_vec->chunk_size_in_bytes ) { go_BYE(-1); }
    ptr_chunk->data = l_malloc(ptr_vec->chunk_size_in_bytes);
    return_if_malloc_failed( ptr_chunk->data);
    memcpy( ptr_chunk->data, X, nX);
    munmap(X, nX);
    // TODO P1 Need to set num_in_chunk
  }
  else {
    //-- Get the chunk from vector's backup file if it exists
    if ( !ptr_vec->is_file ) { go_BYE(-1); }
    char file_name[Q_MAX_LEN_FILE_NAME+1];
    status = mk_file_name(ptr_vec->uqid, file_name); cBYE(status);
    char *X = NULL; size_t nX = 0;
    status = rs_mmap(file_name, &X, &nX, 0); cBYE(status);
    if ( X == NULL ) { go_BYE(-1); }
    if ( nX != ptr_vec->file_size ) { go_BYE(-1); }
    ptr_chunk->data = l_malloc(ptr_vec->chunk_size_in_bytes);
    return_if_malloc_failed( ptr_chunk->data);
    size_t offset = ptr_vec->chunk_size_in_bytes*chunk_num;
    memcpy( ptr_chunk->data, X+offset, nX);
    munmap(X, nX);
  }
BYE:
  return status;
}

int
chk_chunk(
      uint32_t chunk_dir_idx
      )
{
  int status = 0;
  if ( chunk_dir_idx >= g_sz_chunk_dir ) { go_BYE(-1); }
  CHUNK_REC_TYPE *ptr_chunk = g_chunk_dir + chunk_dir_idx;
  char file_name[Q_MAX_LEN_FILE_NAME+1];
  status = mk_file_name(ptr_chunk->uqid, file_name); cBYE(status);
  if ( ptr_chunk->uqid == 0 ) { // we expect this to be free 
    if ( ptr_chunk->num_in_chunk != 0 ) { go_BYE(-1); }
    if ( ptr_chunk->chunk_num != 0 ) { go_BYE(-1); }
    if ( ptr_chunk->uqid != 0 ) { go_BYE(-1); }
    if ( ptr_chunk->vec_uqid != 0 ) { go_BYE(-1); }
    if ( ptr_chunk->is_file ) { go_BYE(-1); }
    if ( isfile(file_name) ) { go_BYE(-1); }
    if ( ptr_chunk->data != NULL ) { go_BYE(-1); }
  }
  else {
    if ( ptr_chunk->data == NULL ) { go_BYE(-1); }
    if ( ptr_chunk->num_in_chunk == 0 ) { 
      // if no data, then can be no file
      if ( ptr_chunk->is_file ) { go_BYE(-1); }
    }
    if ( ptr_chunk->is_file ) { 
      if ( !isfile(file_name) ) { go_BYE(-1); }
    }
    else {
      if ( isfile(file_name) ) { go_BYE(-1); }
    }
  }
BYE:
  return status;
}

static int
allocate_chunk_dir(
    )
{
  int status = 0;
  g_chunk_size = Q_CHUNK_SIZE;
  g_chunk_dir = calloc(NUM_CHUNKS_TO_ALLOCATE, sizeof(CHUNK_REC_TYPE));
  return_if_malloc_failed(g_chunk_dir);
  g_sz_chunk_dir = NUM_CHUNKS_TO_ALLOCATE;
  g_n_chunk_dir = 0;
BYE:
  return status;
}

static int
chk_space_in_chunk_dir(
    )
{
  int status = 0;
  if ( g_chunk_dir == NULL ) {
    status =  allocate_chunk_dir(); cBYE(status);
  }
  else {
    if ( g_n_chunk_dir == g_sz_chunk_dir ) { 
      fprintf(stderr, "Need to allocate space\n"); go_BYE(-1);
    }
  }
BYE:
  return status;
}

int
allocate_chunk(
 size_t sz,
 uint32_t *ptr_idx
    )
{
  int status = 0;
  *ptr_idx = 0;
  if ( sz == 0 ) { 
    printf("hello world\n"); 
    go_BYE(-1); }
  status = chk_space_in_chunk_dir(); cBYE(status);
  // we do not allocate 0th entry
  for ( unsigned int i = 1 ; i < g_sz_chunk_dir; i++ ) { 
    if ( g_chunk_dir[i].uqid == 0 ) {
      g_chunk_dir[i].uqid = RDTSC(); 
      g_chunk_dir[i].data = malloc(sz);
      return_if_malloc_failed(g_chunk_dir[i].data);
      *ptr_idx = i;
      break;
    }
  }
  if ( *ptr_idx == 0 ) { 
    fprintf(stderr, "No space in chunk directory\n"); go_BYE(-1); 
  }
BYE:
  return status; 
}

int64_t 
get_exp_file_size(
    uint64_t num_elements,
    uint32_t field_width,
    const char * const fldtype
    )
{
  int64_t expected_file_size = num_elements * field_width;
  if ( strcmp(fldtype, "B1") == 0 ) {
    uint64_t num_words = num_elements / 64;
    if ( ( num_words * 64 ) != num_elements ) { num_words++; }
    expected_file_size = num_words * 8;
  }
  return expected_file_size;
}

int32_t
get_chunk_size(
      uint32_t field_width, 
      const char * const field_type
      )
{
  int32_t chunk_size_in_bytes = g_chunk_size * field_width;
  if ( strcmp(field_type, "B1") == 0 ) {  // SPECIAL CASE
    chunk_size_in_bytes = g_chunk_size / 8;
    if ( ( ( g_chunk_size / 64 ) * 64 ) != g_chunk_size ) { 
      WHEREAMI; return -1; 
    }
  }
  return chunk_size_in_bytes;
}

int
as_hex(
    uint64_t n,
    char *buf
    )
{
  int status = 0;
  for ( int i = 0; i < 16; i++ ) { 
    char c;
    switch ( n & 0xF )  {
      case 0 : c = '0'; break;
      case 1 : c = '1'; break;
      case 2 : c = '2'; break;
      case 3 : c = '3'; break;
      case 4 : c = '4'; break;
      case 5 : c = '5'; break;
      case 6 : c = '6'; break;
      case 7 : c = '7'; break;
      case 8 : c = '8'; break;
      case 9 : c = '9'; break;
      case 10 : c = 'A'; break;
      case 11 : c = 'B'; break;
      case 12 : c = 'C'; break;
      case 13 : c = 'D'; break;
      case 14 : c = 'E'; break;
      case 15 : c = 'F'; break;
      default : go_BYE(-1); break; 
    }
    buf[16-1-i] = c;
    n = n >> 4;
  }
BYE:
  return status;
}

int
mk_file_name(
    uint64_t uqid, 
    char *file_name
    )
{
  int status = 0;
  char buf[32];
  status = as_hex(uqid, buf); cBYE(status);
  snprintf(buf, Q_MAX_LEN_FILE_NAME, "%s/_%s.bin", g_q_data_dir, buf);
BYE:
  return status;
}
