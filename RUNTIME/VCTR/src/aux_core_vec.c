#include "q_incs.h"
#include "vec_macros.h"
#include "vctr_struct.h"

#include "get_file_size.h"
#include "copy_file.h"
#include "isfile.h"
#include "isdir.h"
#include "rdtsc.h"
#include "rs_mmap.h"

#define Q_MAX_LEN_FILE_NAME 63 // TODO  P2 Delete
#define CORE_VEC_ALIGNMENT 64
#include "aux_core_vec.h"

uint64_t
mk_uqid(
    qmem_struct_t *ptr_S
    )
{
  uint64_t n = ptr_S->max_file_num + 1;
  ptr_S->max_file_num = n;
  return n;
}

int
safe_strcat(
    char **ptr_X,
    size_t *ptr_nX,
    const char * const buf
    )
{
  int status = 0;
  char *X = *ptr_X;
  size_t nX = *ptr_nX;
  size_t buflen = strlen(buf);
  size_t Xlen   = strlen(X);
  if ( Xlen + buflen + 2 >= nX ) { 
    while ( (Xlen + buflen + 2) >= nX ) { 
      nX *= 2;
    }
    X = malloc(nX);
    return_if_malloc_failed(X);
    memset(X, 0, nX);
    strcpy(X, *ptr_X);
  }
  strcat(X, buf);
  *ptr_X = X;
  *ptr_nX = nX;
BYE:
  return status;
}


void 
l_memcpy(
    void *dest,
    const void *src,
    size_t n
    )
{
  memcpy(dest, src, n);
}

void *
l_malloc(
    size_t n
    )
{
  int status = 0;
  void  *x = NULL;

  status = posix_memalign(&x, CORE_VEC_ALIGNMENT, n); 
  if ( status < 0 ) { WHEREAMI; return NULL; }
  if ( x == NULL ) { WHEREAMI; return NULL; }
  if ( status < 0 ) { WHEREAMI; return NULL; }

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
  for ( const char *cptr = (const char *)name; *cptr != '\0'; cptr++ ) { 
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
chk_fldtype(
    const char * const fldtype,
    uint32_t field_width
    )
{
  int status = 0;
  if ( fldtype == NULL ) { go_BYE(-1); }
  // TODO P3: SYNC with qtypes in q_consts.lua
  if ( ( strcmp(fldtype, "B1") == 0 ) || 
       ( strcmp(fldtype, "I1") == 0 ) || 
       ( strcmp(fldtype, "I2") == 0 ) || 
       ( strcmp(fldtype, "I4") == 0 ) || 
       ( strcmp(fldtype, "I8") == 0 ) || 
       ( strcmp(fldtype, "F4") == 0 ) || 
       ( strcmp(fldtype, "F8") == 0 ) || 
       ( strcmp(fldtype, "SC") == 0 ) || 
       ( strcmp(fldtype, "TM") == 0 ) ) {
    /* all is well */
  }
  else {
    fprintf(stderr, "Bad field type = [%s] \n", fldtype);
    go_BYE(-1);
  }
  if ( strcmp(fldtype, "B1") == 0 )  {
    if ( field_width != 1 ) { go_BYE(-1); }
  }
  else {
    if ( field_width == 0 ) { go_BYE(-1); }
  }
  if ( strcmp(fldtype, "SC") == 0 )  {
    if ( field_width < 2 ) { go_BYE(-1); }
  }
BYE:
  return status;
}

int
load_chunk(
    const CHUNK_REC_TYPE *const chunk, 
    const VEC_REC_TYPE *const ptr_vec,
    uint64_t *ptr_t_last_get,
    char **ptr_data
    )
{
  int status = 0;
  char *data = NULL;
  *ptr_t_last_get = RDTSC();
  if ( chunk->data != NULL ) { return status; } // already loaded
  // double check that this chunk is yours
  if ( chunk->vec_uqid != ptr_vec->uqid ) { go_BYE(-1); }
  if ( chunk->num_readers != 0 ) { go_BYE(-1); }

  // must be able to backup data from chunk file or vector file 
  if ( ( !chunk->is_file ) && ( !ptr_vec->is_file ) ) { go_BYE(-1); }

  char file_name[Q_MAX_LEN_FILE_NAME+1];
  memset(file_name, '\0', Q_MAX_LEN_FILE_NAME+1);
  char *X = NULL; size_t nX = 0;
  data = l_malloc(ptr_vec->chunk_size_in_bytes);
  return_if_malloc_failed(data);
  memset(data, '\0', ptr_vec->chunk_size_in_bytes);

  if ( chunk->is_file ) { // chunk has a backup file 
    status = mk_file_name(chunk->uqid, file_name, Q_MAX_LEN_FILE_NAME);
    cBYE(status);
    status = rs_mmap(file_name, &X, &nX, 0); cBYE(status);
    if ( X == NULL ) { go_BYE(-1); }
    if ( nX > ptr_vec->chunk_size_in_bytes ) { go_BYE(-1); }
    memcpy(data, X, nX);
  }
  else { // vector has a backup file 
    size_t num_to_copy = ptr_vec->chunk_size_in_bytes;
    status = mk_file_name(ptr_vec->uqid, file_name, Q_MAX_LEN_FILE_NAME); 
    cBYE(status);
    status = rs_mmap(file_name, &X, &nX, 0); cBYE(status);
    if ( X == NULL                ) { go_BYE(-1); }
    if ( nX != ptr_vec->file_size ) { go_BYE(-1); }
    size_t offset = ptr_vec->chunk_size_in_bytes * chunk->chunk_num;
    // handle case where last chunk requested and vec_size not multiple 
    if ( nX - offset < num_to_copy ) { num_to_copy = nX - offset; }
    //--------
    if ( offset + num_to_copy > nX ) { go_BYE(-1); }
    memcpy(data, X + offset, num_to_copy);
  }
  *ptr_data = data;
  munmap(X, nX);
BYE:
  return status;
}

int
chk_chunk(
    const qmem_struct_t *ptr_S,
    const VEC_REC_TYPE *const ptr_vec,
    uint32_t chunk_dir_idx,
    uint64_t vec_uqid
    )
{
  int status = 0;
  if ( chunk_dir_idx >= ptr_S->chunk_dir->sz ) { go_BYE(-1); }
  CHUNK_REC_TYPE *chunk = ptr_S->chunk_dir->chunks + chunk_dir_idx;
  /* What checks on these guys?
  if ( ptr_vec->num_readers > 0 ) { go_BYE(-1); }
  */
  char file_name[Q_MAX_LEN_FILE_NAME+1];
  memset(file_name, '\0', Q_MAX_LEN_FILE_NAME+1);
  status = mk_file_name(chunk->uqid, file_name, Q_MAX_LEN_FILE_NAME); 
  cBYE(status);
  if ( chunk->uqid == 0 ) { // we expect this to be free 
    if ( chunk->chunk_num != 0 ) { go_BYE(-1); }
    if ( chunk->uqid != 0 ) { go_BYE(-1); }
    if ( chunk->vec_uqid != 0 ) { go_BYE(-1); }
    if ( chunk->is_file ) { go_BYE(-1); }
    if ( isfile(file_name) ) { go_BYE(-1); }
    if ( chunk->data != NULL ) { go_BYE(-1); }
  }
  else {
    if ( chunk->vec_uqid != vec_uqid ) { go_BYE(-1); } }
    if ( !ptr_vec->is_file ) { 
      // this check is valid only when there is no master file 
      if ( chunk->data == NULL ) { 
        if ( !chunk->is_file ) { go_BYE(-1); }
      }
    }
    if ( chunk->is_file ) { 
      if ( !isfile(file_name) ) { }
  }
BYE:
  return status;
}

static int
chk_space_in_chunk_dir(
    const qmem_struct_t *ptr_S
    )
{
  int status = 0;
  if ( ptr_S->chunk_dir == NULL ) { go_BYE(-1); }
  if ( ptr_S->chunk_dir->n >= ptr_S->chunk_dir->sz ) { 
    // TODO P1
    fprintf(stderr, "TO BE IMPLEMENTED: allocate space\n"); go_BYE(-1);
  }
BYE:
  return status;
}

int
allocate_chunk(
    const qmem_struct_t *ptr_S,
    size_t sz,
    uint32_t chunk_num,
    uint64_t vec_uqid,
    uint32_t *ptr_chunk_dir_idx,
    bool is_malloc
    )
{
  int status = 0;
  static unsigned int start_search = 1;
  status = chk_space_in_chunk_dir(ptr_S);  cBYE(status); 
  // NOTE: we do not allocate 0th entry
  for ( int iter = 0; iter < 2; iter++ ) { 
    unsigned int lb, ub;
    if ( iter == 0 ) { 
      lb = start_search; // note not 0
      ub = ptr_S->chunk_dir->sz;
    }
    else {
      lb = 1; 
      ub = start_search; // note not 0
    }
    for ( unsigned int i = lb ; i < ub; i++ ) { 
      if ( ptr_S->chunk_dir->chunks[i].uqid == 0 ) {
        ptr_S->chunk_dir->chunks[i].uqid = mk_uqid((qmem_struct_t *)ptr_S);
        ptr_S->chunk_dir->chunks[i].chunk_num = chunk_num;
        ptr_S->chunk_dir->chunks[i].is_file = false;
        ptr_S->chunk_dir->chunks[i].vec_uqid = vec_uqid;
        if ( is_malloc ) { 
          if ( sz == 0 ) { go_BYE(-1); }
          ptr_S->chunk_dir->chunks[i].data = l_malloc(sz);
          return_if_malloc_failed(ptr_S->chunk_dir->chunks[i].data);
        }
        *ptr_chunk_dir_idx = i; 
        ptr_S->chunk_dir->n++;
        start_search = i+1;
        if ( start_search >= ptr_S->chunk_dir->sz ) { start_search = 1; }
        return status;
      }
    }
  }
  /* control should not come here */
  go_BYE(-1); 
BYE:
  return status;
}

int64_t 
get_exp_file_size(
    const qmem_struct_t *ptr_S,
    uint64_t num_elements,
    uint32_t field_width,
    const char * const fldtype
    )
{
  // TODO: Currently we write entire chunk even if partially used
  num_elements = ceil((double)num_elements / ptr_S->chunk_size) * ptr_S->chunk_size;
  int64_t expected_file_size = num_elements * field_width;
  if ( strcmp(fldtype, "B1") == 0 ) {
    uint64_t num_words = num_elements / 64;
    if ( ( num_words * 64 ) != num_elements ) { num_words++; }
    expected_file_size = num_words * 8;
  }
  return expected_file_size;
}

int32_t
get_chunk_size_in_bytes(
    const qmem_struct_t *ptr_S,
      uint32_t field_width, 
      const char * const fldtype
      )
{
  int32_t chunk_size_in_bytes = ptr_S->chunk_size * field_width;
  if ( ptr_S->chunk_size == 0 ) { WHEREAMI; return -1; }
  if ( strcmp(fldtype, "B1") == 0 ) {  // SPECIAL CASE
    chunk_size_in_bytes = ptr_S->chunk_size / 8;
    if ( ( ( ptr_S->chunk_size / 64 ) * 64 ) != ptr_S->chunk_size ) { 
      WHEREAMI; return -1; 
    }
  }
  return chunk_size_in_bytes;
}

int
as_hex(
    uint64_t n,
    char *buf,
    size_t buflen
    )
{
  int status = 0;
  if ( buflen < 16+1 ) { go_BYE(-1); }
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
    char *file_name, // [sz]
    int len_file_name
    )
{
  int status = 0;
  int len = NUM_HEX_DIGITS_IN_UINT64;
  char buf[len+1];
  memset(buf, '\0', len+1);
  if ( len_file_name > 0 ) { memset(file_name, '\0', len_file_name+1); }
  status = as_hex(uqid, buf, len); cBYE(status);
  // TODO P3 Need to avoid repeated initialization
  char *data_dir = getenv("Q_DATA_DIR");
  if ( data_dir == NULL ) { go_BYE(-1); }
  if ( !isdir(data_dir) ) { go_BYE(-1); }
  int nw = snprintf(file_name, Q_MAX_LEN_FILE_NAME, 
      "%s/_%s.bin", data_dir, buf);
  if ( nw == Q_MAX_LEN_FILE_NAME ) { go_BYE(-1); }
BYE:
  return status;
}

int
init_chunk_dir(
    VEC_REC_TYPE *ptr_vec,
    int num_chunks
    )
{
  int status = 0;
  // if elements exist, you would have initialized directory
  if ( ptr_vec->num_elements != 0 ) { return status; }
  //-----------------------------------------
  if ( ptr_vec->chunks     != NULL ) { go_BYE(-1); }
  if ( ptr_vec->sz_chunks  != 0    ) { go_BYE(-1); }
  if ( ptr_vec->num_chunks != 0    ) { go_BYE(-1); }
  if ( num_chunks < 0 ) { 
    if ( ptr_vec->is_memo ) { 
      num_chunks = 1;
    }
    else {
      num_chunks = INITIAL_NUM_CHUNKS_PER_VECTOR;
    }
  }
  ptr_vec->chunks = calloc(num_chunks, sizeof(uint32_t));
  return_if_malloc_failed(ptr_vec->chunks);
  ptr_vec->sz_chunks = num_chunks;
BYE:
  return status;
}

// tells us which chunk to read this element from
int 
chunk_dir_idx_for_read(
    const qmem_struct_t *ptr_S,
    VEC_REC_TYPE *ptr_vec,
    uint64_t idx,
    uint32_t *ptr_chunk_dir_idx
    )
{
  int status = 0;
  if ( ptr_vec->num_elements == 0 ) { go_BYE(-1); }
  if ( idx >= ptr_vec->num_elements ) { go_BYE(-1); }
  uint32_t chunk_num;
  if ( !ptr_vec->is_memo ) { 
    chunk_num = 0; 
  }
  else {
    chunk_num = idx / ptr_S->chunk_size;
  }
  if ( chunk_num >= ptr_vec->num_chunks ) { go_BYE(-1); }
  *ptr_chunk_dir_idx = ptr_vec->chunks[chunk_num];
  if ( *ptr_chunk_dir_idx >= ptr_S->chunk_dir->sz ) { go_BYE(-1); }
BYE:
  return status;
}
// tells us which chunk to write this element into
int 
get_chunk_num_for_write(
    const qmem_struct_t *ptr_S,
    VEC_REC_TYPE *ptr_vec,
    uint32_t *ptr_chunk_num
    )
{
  int status = 0;
  uint32_t *new = NULL;
  if ( !ptr_vec->is_memo ) { *ptr_chunk_num = 0; return status; }
  // let us say chunk size = 64 and num elements = 63
  // this means that when you want to write, you write to chunk 0
  // let us say chunk size = 64 and num elements = 64
  // this means that when you want to write, you write to chunk 1
  uint32_t chunk_num =  (ptr_vec->num_elements / ptr_S->chunk_size);
  uint32_t sz = ptr_vec->sz_chunks;
  if ( chunk_num >= sz ) { // need to reallocate space
    new = calloc(2*sz, sizeof(uint32_t));
    return_if_malloc_failed(new);
    for ( uint32_t i = 0; i < sz; i++ ) {
      new[i] = ptr_vec->chunks[i];
    }
    free(ptr_vec->chunks);
    ptr_vec->chunks  = new;
    ptr_vec->sz_chunks  = 2*sz;
  }
  *ptr_chunk_num = chunk_num;
BYE:
  return status;
}

int 
get_chunk_dir_idx(
    const qmem_struct_t *ptr_S,
    const VEC_REC_TYPE *const ptr_vec,
    uint32_t chunk_num,
    uint32_t *ptr_num_chunks,
    uint32_t *ptr_chunk_dir_idx,
    bool is_malloc
    )
{
  int status = 0;
  if ( chunk_num >= ptr_vec->sz_chunks )  { go_BYE(-1); }
  uint32_t chunk_dir_idx = ptr_vec->chunks[chunk_num];
  if ( chunk_dir_idx == 0 ) { // we need to set it 
    status = allocate_chunk(ptr_S, ptr_vec->chunk_size_in_bytes, 
        chunk_num, ptr_vec->uqid, &chunk_dir_idx, is_malloc); 
    cBYE(status);
    *ptr_num_chunks = *ptr_num_chunks + 1;
  }
  *ptr_chunk_dir_idx = chunk_dir_idx;
  if ( chunk_dir_idx >= ptr_S->sz_chunk_dir ) { go_BYE(-1); }
  ptr_vec->chunks[chunk_num] = chunk_dir_idx;
BYE:
  return status;
}

int
vec_new_common(
    VEC_REC_TYPE *ptr_vec,
    const char * const fldtype,
    uint32_t field_width
    )
{
  int status = 0;
  if ( ptr_vec == NULL ) { go_BYE(-1); }
  status = chk_fldtype(fldtype, field_width); cBYE(status);

  memset(ptr_vec, '\0', sizeof(VEC_REC_TYPE));

  strncpy(ptr_vec->fldtype, fldtype, Q_MAX_LEN_QTYPE_NAME-1);
  ptr_vec->field_width = field_width;
  ptr_vec->chunk_size_in_bytes = get_chunk_size_in_bytes(
      ptr_S, field_width, fldtype);
  ptr_vec->uqid = mk_uqid(ptr_S);
  //-----------------------------
  chunk_dir = malloc(1 * sizeof(chunk_dir_t));
  return_if_malloc_failed(chunk_dir);
  memset(chunk_dir, '\0', sizeof(chunk_dir_t));
  chunk_dir->n = 0;
  chunk_dir->chunks = malloc(chunk_dir->sz * sizeof(CHUNK_REC_TYPE));
  return_if_malloc_failed(chunk_dir->chunks);
  chunk_dir->sz = Q_INITIAL_SZ_CHUNK_DIR;
  ptr_vec->chunk_dir = chunk_dir; 
  //-----------------------------


  ptr_vec->is_memo = true; // default behavior
BYE:
  return status;
}
//---------------------
int
delete_chunk_file(
    const CHUNK_REC_TYPE *ptr_chunk,
    bool *ptr_is_file
    )
{
  int status = 0;
  if ( ptr_chunk->is_file ) {
    char file_name[Q_MAX_LEN_FILE_NAME+1];
    status = mk_file_name(ptr_chunk->uqid, file_name, Q_MAX_LEN_FILE_NAME); 
    cBYE(status);
    if ( !isfile(file_name) ) { go_BYE(-1); }
    status = remove(file_name); cBYE(status);
  }
BYE:
  *ptr_is_file = false;
  return status;
}

int 
make_master_file(
    const qmem_struct_t *ptr_S,
    VEC_REC_TYPE *ptr_v,
    bool is_free_mem
    )
{
  int status = 0; 
  FILE *vfp = NULL; 
  size_t file_size = 0; // number of bytes in file 
  char *X = NULL; size_t nX = 0;
  char vfile_name[Q_MAX_LEN_FILE_NAME+1];
  if ( !ptr_v->is_eov ) { go_BYE(-1); }
  if ( ptr_v->is_file ) { 
    // Nothing to do: file exists 
    return status; 
  }
  status = mk_file_name(ptr_v->uqid, vfile_name, Q_MAX_LEN_FILE_NAME); 
  cBYE(status);
  vfp = fopen(vfile_name, "wb"); 
  return_if_fopen_failed(vfp, vfile_name, "wb"); 
  for ( unsigned int i = 0; i < ptr_v->num_chunks; i++ ) { 
    int nw;
    char cfile_name[Q_MAX_LEN_FILE_NAME+1];
    uint32_t chunk_idx = ptr_v->chunks[i];
    chk_chunk_idx(chunk_idx);
    CHUNK_REC_TYPE *ptr_chunk = ptr_S->chunk_dir + chunk_idx;
    if ( ptr_chunk->data ) { 
      nw = fwrite(ptr_chunk->data, ptr_v->chunk_size_in_bytes, 1, vfp);
      if ( nw != 1 ) { go_BYE(-1); }
    }
    else {
      status = mk_file_name(ptr_chunk->uqid, cfile_name, Q_MAX_LEN_FILE_NAME);
      cBYE(status);
      if ( nX != ptr_v->chunk_size_in_bytes ) { go_BYE(-1); }
      status = rs_mmap(cfile_name, &X, &nX, 0); cBYE(status);
      nw = fwrite(X, nX, 1, vfp);
      if ( nw != 1 ) { go_BYE(-1); }
      munmap(X, nX); X = NULL; nX = 0;
    }
    file_size += ptr_v->chunk_size_in_bytes;
    if ( is_free_mem ) {
      free_if_non_null(ptr_chunk->data);
    }
  }
  ptr_v->is_file = true;
  ptr_v->file_size = file_size;
BYE:
  fclose_if_non_null(vfp);
  return status;
}

int
reincarnate(
    const qmem_struct_t *ptr_S,
    const VEC_REC_TYPE *const ptr_v,
    char **ptr_X,
    bool is_clone
    )
{
  int status = 0;
  size_t nX = 65536;
#define BUFLEN 65535
  char *buf = NULL;
  char *X = NULL;

  // check status of vector 
  if ( ptr_v->is_dead ) { go_BYE(-1); }
  if ( ptr_v->num_writers > 0 ) { go_BYE(-1); }
  if ( !ptr_v->is_eov ) { go_BYE(-1); }
  //--------------
  buf = malloc(BUFLEN+1);
  return_if_malloc_failed(buf);
  memset(buf, '\0', BUFLEN+1);

  X = malloc(nX);
  return_if_malloc_failed(X);
  memset(X, '\0', nX);

  strcpy(buf, " return { ");
  safe_strcat(&X, &nX, buf);

  sprintf(buf, "qtype = \"%s\", ", ptr_v->fldtype); 
  safe_strcat(&X, &nX, buf);

  sprintf(buf, "num_elements = %" PRIu64 ", ", ptr_v->num_elements); 
  safe_strcat(&X, &nX, buf);

  sprintf(buf, "chunk_size = %d, ", ptr_S->chunk_size);
  safe_strcat(&X, &nX, buf);

  sprintf(buf, "width = %u, ", ptr_v->field_width); 
  safe_strcat(&X, &nX, buf);

  char old_file_name[Q_MAX_LEN_FILE_NAME+1];
  char new_file_name[Q_MAX_LEN_FILE_NAME+1];

  uint64_t old_vec_uqid = ptr_v->uqid; 
  status = mk_file_name(old_vec_uqid, old_file_name,Q_MAX_LEN_FILE_NAME); 
  cBYE(status);
  if ( is_clone ) { 
    uint64_t new_vec_uqid = mk_uqid(ptr_S);
    if ( ptr_v->is_file ) {
      status = mk_file_name(new_vec_uqid, new_file_name,Q_MAX_LEN_FILE_NAME); 
      status = copy_file(old_file_name, new_file_name); cBYE(status);
    }
    sprintf(buf, "vec_uqid = %" PRIu64 ",",  new_vec_uqid);
  }
  else {
    sprintf(buf, "vec_uqid = %" PRIu64 ",",  old_vec_uqid);
  }
  safe_strcat(&X, &nX, buf);
  //-------------------------------------------------------
  // no master file => either 
  // (1) all chunk files must exist. 
  // (2) chunk data must exist 
  if ( !ptr_v->is_file ) { 
    for ( unsigned int i = 0; i < ptr_v->num_chunks; i++ ) { 
      uint32_t chunk_idx = ptr_v->chunks[i];
      chk_chunk_idx(chunk_idx);
      CHUNK_REC_TYPE *ptr_c = ptr_S->chunk_dir + chunk_idx;
      uint64_t old_uqid = ptr_c->uqid; 
      status = mk_file_name(old_uqid, old_file_name,Q_MAX_LEN_FILE_NAME); 
      cBYE(status);
      if ( ( !isfile(old_file_name) ) && ( ptr_c->data == NULL ) ) { 
        go_BYE(-1);
      }
    }
  }
  //------------------------------------------------------------
  safe_strcat(&X, &nX, "chunk_uqids = { ");
  for ( unsigned int i = 0; i < ptr_v->num_chunks; i++ ) { 
    uint32_t chunk_idx = ptr_v->chunks[i];
    chk_chunk_idx(chunk_idx);
    CHUNK_REC_TYPE *ptr_c = ptr_S->chunk_dir + chunk_idx;
    uint64_t old_uqid = ptr_c->uqid; 
    status = mk_file_name(old_uqid, old_file_name,Q_MAX_LEN_FILE_NAME); 
    cBYE(status);
    if ( is_clone ) { 
      uint64_t new_uqid = mk_uqid(ptr_S);
      status = mk_file_name(new_uqid, new_file_name,Q_MAX_LEN_FILE_NAME); 
      if ( ptr_c->is_file ) { 
        status = copy_file(old_file_name, new_file_name); cBYE(status);
      }
      sprintf(buf, "%" PRIu64 ",",  new_uqid);
    }
    else {
      sprintf(buf, "%" PRIu64 ",",  old_uqid);
    }
    safe_strcat(&X, &nX, buf);
  }
  safe_strcat(&X, &nX, " }  ");
  safe_strcat(&X, &nX, " }  ");
  *ptr_X = X;
BYE:
  free_if_non_null(buf);
  if ( status < 0 ) { free_if_non_null(X); }
  return status;
}

bool
is_multiple(
    uint64_t x, 
    uint32_t y
    )
{
  if ( ( ( x /y ) * y ) == x ) { return true; } else { return false; }
}
//------------------------------------------------------
// This should be invoked only when a master file exists 
int
vec_clean_chunks(
    const qmem_struct_t *ptr_S,
    const VEC_REC_TYPE *const ptr_vec
    )
{
  int status = 0;
  if ( ptr_vec == NULL ) { go_BYE(-1); }
  if ( ptr_vec->is_eov == false ) { go_BYE(-1); }
  if ( ptr_vec->is_file == false ) { go_BYE(-1); }
  uint32_t lb = 0;
  uint32_t ub = ptr_vec->num_chunks; 
  for ( unsigned int i = lb; i < ub; i++ ) { 
    char file_name[Q_MAX_LEN_FILE_NAME+1];
    uint32_t chunk_idx = ptr_vec->chunks[i];
    chk_chunk_idx(chunk_idx);
    CHUNK_REC_TYPE *ptr_chunk = ptr_S->chunk_dir + chunk_idx;
    if ( ptr_chunk->is_file ) { // flush buffer only if NO backup 
      memset(file_name, '\0', Q_MAX_LEN_FILE_NAME+1);
      status = mk_file_name(ptr_chunk->uqid, file_name, Q_MAX_LEN_FILE_NAME);
      cBYE(status);
      status = remove(file_name); cBYE(status);
      ptr_chunk->is_file = false;
    }
    free_if_non_null(ptr_chunk->data);
  }
BYE:
  return status;
}

