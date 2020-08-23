#include "q_incs.h"
#include "vec_macros.h"
#include "vctr_struct.h"

#include "get_file_size.h"
#include "copy_file.h"
#include "isfile.h"
#include "isdir.h"
#include "rdtsc.h"
#include "rs_mmap.h"

#include "aux_qmem.h"
#include "aux_core_vec.h"

uint64_t
get_uqid(
    qmem_struct_t *ptr_S
    )
{
  return ++ptr_S->uqid_gen;
}

int
free_chunk(
    const qmem_struct_t *ptr_S,
    uint32_t chunk_dir_idx,
    bool is_persist
    )
{
  int status = 0;
  chk_chunk_idx(chunk_dir_idx); 

  CHUNK_REC_TYPE *chunk =  ptr_S->chunk_dir->chunks + chunk_dir_idx;
  if ( chunk->num_readers > 0 ) { go_BYE(-1); }
  free_if_non_null(chunk->data);
  if ( !is_persist ) { 
    status = delete_chunk_file(chunk, &(chunk->is_file)); cBYE(status);
  }
  memset(chunk, '\0', sizeof(CHUNK_REC_TYPE));
  ptr_S->chunk_dir->n--;
BYE:
  return status;
}

int
chk_chunk(
    const qmem_struct_t *ptr_S,
    const VEC_REC_TYPE *const v,
    uint32_t chunk_dir_idx
    )
{
  int status = 0;
  char *file_name = NULL;

  WHOLE_VEC_REC_TYPE *w = 
    ptr_S->whole_vec_dir->whole_vecs + v->whole_vec_dir_idx;
  if ( w->uqid != v->uqid ) { go_BYE(-1); } 
  if ( chunk_dir_idx >= ptr_S->chunk_dir->sz ) { go_BYE(-1); }
  CHUNK_REC_TYPE *chunk = ptr_S->chunk_dir->chunks + chunk_dir_idx;
  /* What checks on these guys?
     if ( ptr_vec->num_readers > 0 ) { go_BYE(-1); }
     */
  status = mk_file_name(ptr_S, chunk->uqid, &file_name);
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
    if ( chunk->vec_uqid != v->uqid ) { go_BYE(-1); }
    if ( !w->is_file ) { 
      // this check is valid only when there is no master file 
      if ( chunk->data == NULL ) { 
        if ( !chunk->is_file ) { go_BYE(-1); }
      }
    }
    if ( chunk->is_file ) { 
      if ( !isfile(file_name) ) { 
        // TODO P1 WHAT HAPPENS HEER 
      }
    }
  }
BYE:
  free_if_non_null(file_name);
  return status;
}

int
allocate_chunk(
    qmem_struct_t *ptr_S,
    VEC_REC_TYPE *v,
    uint32_t chunk_num,
    uint32_t *ptr_chunk_dir_idx,
    bool is_malloc
    )
{
  int status = 0;
  static unsigned int start_search = 1;
  size_t sz = ptr_S->chunk_size * v->field_width;
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
        ptr_S->chunk_dir->chunks[i].vec_uqid = v->uqid;
        if ( is_malloc ) { 
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

size_t
get_chunk_size_in_bytes(
    const qmem_struct_t *ptr_S,
    VEC_REC_TYPE *ptr_v
    )
{
  size_t chunk_size_in_bytes = ptr_S->chunk_size * ptr_v->field_width;
  if ( ptr_S->chunk_size == 0 ) { WHEREAMI; return -1; }
  if ( strcmp(ptr_v->fldtype, "B1") == 0 ) {  // SPECIAL CASE
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
    const qmem_struct_t *ptr_S,
    uint64_t uqid, 
    char **ptr_file_name // [sz]
    )
{
  int status = 0;
  char *file_name = NULL;
  char buf[NUM_HEX_DIGITS_IN_UINT64+1];
  memset(buf, '\0', NUM_HEX_DIGITS_IN_UINT64+1);
  status = as_hex(uqid, buf, NUM_HEX_DIGITS_IN_UINT64); cBYE(status);
  // TODO P3 Need to avoid repeated initialization
  char *data_dir = ptr_S->q_data_dir; 
  if ( data_dir == NULL ) { go_BYE(-1); }
  if ( !isdir(data_dir) ) { go_BYE(-1); }
  int len = strlen(data_dir) + 64; 
  file_name = malloc(len * sizeof(char));
  return_if_malloc_failed(file_name);
  snprintf(file_name, len-1, "%s/_%s.bin", data_dir, buf);
  *ptr_file_name = file_name;
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
    VEC_REC_TYPE *ptr_vec,
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
    status = allocate_chunk((qmem_struct_t *)ptr_S, ptr_vec, chunk_num, 
        &chunk_dir_idx, is_malloc); 
    cBYE(status);
    *ptr_num_chunks = *ptr_num_chunks + 1;
  }
  *ptr_chunk_dir_idx = chunk_dir_idx;
  if ( chunk_dir_idx >= ptr_S->chunk_dir->sz ) { go_BYE(-1); }
  ptr_vec->chunks[chunk_num] = chunk_dir_idx;
BYE:
  return status;
}

int
qmem_unload_chunk(
    const qmem_struct_t *ptr_S,
    const VEC_REC_TYPE *const ptr_vec,
    int chunk_num
    )
{
  int status = 0;
BYE:
  return status;
}

int
qmem_backup_chunk(
    const qmem_struct_t *ptr_S,
    const VEC_REC_TYPE *const ptr_vec,
    int chunk_num
    )
{
  int status = 0;
  char *file_name = NULL;
  if ( ptr_vec == NULL ) { go_BYE(-1); }
  if ( ptr_vec->is_eov == false ) { go_BYE(-1); }
  uint32_t chunk_idx = ptr_vec->chunks[chunk_num];
  chk_chunk_idx(chunk_idx);
  CHUNK_REC_TYPE *chunk = ptr_S->chunk_dir->chunks + chunk_idx;
  //  if file exists, nothing to do 
  if ( chunk->is_file ) { return status; } 

  status = mk_file_name(ptr_S, chunk->uqid, &file_name); cBYE(status);
  size_t chnk_sz = get_chunk_size_in_bytes(ptr_S, ptr_vec);
  if ( chunk->data == NULL ) { go_BYE(-1); }
  FILE *fp = fopen(file_name, "wb");
  return_if_fopen_failed(fp, file_name, "wb");
  fwrite(chunk->data, chnk_sz, 1, fp);
  fclose(fp);
  chunk->is_file = true;
BYE:
  free_if_non_null(file_name);
  return status;
}

int
qmem_backup_chunks(
    const qmem_struct_t *ptr_S,
    const VEC_REC_TYPE *const ptr_vec
    )
{
  int status = 0;
  if ( ptr_vec == NULL ) { go_BYE(-1); }
  if ( ptr_vec->is_eov == false ) { go_BYE(-1); }

  for ( uint32_t i = 0; i  < ptr_vec->num_chunks; i++ ) { 
    status = qmem_backup_chunk(ptr_S, ptr_vec, ptr_vec->chunks[i]);
    cBYE(status);
  }
BYE:
  return status;
}
int
qmem_un_backup_vec(
    const qmem_struct_t *ptr_S,
    VEC_REC_TYPE *v
    )
{
  int status = 0;
  char *file_name = NULL;
  WHOLE_VEC_REC_TYPE *w = 
    ptr_S->whole_vec_dir->whole_vecs + v->whole_vec_dir_idx;
  if ( w->uqid != v->uqid ) { go_BYE(-1); } 

  // No master file => nothing to do 
  if  ( !w->is_file ) { return status; } 

  // Before deleting master file, must make sure chunks contain data
  for ( uint32_t i = 0; i < v->num_chunks; i++ ) { 
    uint32_t chunk_idx = v->chunks[i];
    CHUNK_REC_TYPE *chunk = ptr_S->chunk_dir->chunks + chunk_idx;
    if ( ( chunk->is_file ) || ( chunk->data != NULL ) ) {
      /* all is well */
    }
    else {
      status = load_chunk(chunk, v, &(chunk->t_last_get), &(chunk->data)); 
      cBYE(status);
    }
  }
  status = mk_file_name(ptr_S, v->uqid, &file_name); cBYE(status);
  status = remove(file_name); cBYE(status);
  w->is_file = false;
  w->file_size = 0;
BYE:
  free_if_non_null(file_name);
  return status;
}

int
qmem_un_backup_chunks(
    const qmem_struct_t *ptr_S,
    VEC_REC_TYPE *v
    )
{
  int status = 0;
  char *file_name = NULL;
  WHOLE_VEC_REC_TYPE *w = 
    ptr_S->whole_vec_dir->whole_vecs + v->whole_vec_dir_idx;
  if ( w->uqid != v->uqid ) { go_BYE(-1); } 

  for ( uint32_t i = 0; i < v->num_chunks; i++ ) { 
    uint32_t chunk_idx = v->chunks[i];
    CHUNK_REC_TYPE *chunk = ptr_S->chunk_dir->chunks + chunk_idx;
    if ( !chunk->is_file ) { continue; }
    // At this point, we know that chunk file does not exist 
    // can delete either if data in memory or master file exists
    if  ( ( w->is_file ) || ( chunk->data != NULL ) ) {
      status = mk_file_name(ptr_S, chunk->uqid, &file_name); cBYE(status);
      status = remove(file_name); cBYE(status);
      free_if_non_null(file_name);
      chunk->is_file = false;
    }
  }
BYE:
  free_if_non_null(file_name);
  return status;
}
int 
qmem_backup_vec(
    const qmem_struct_t *ptr_S,
    VEC_REC_TYPE *v
    )
{
  int status = 0; 
  char * vfile_name = NULL;
  FILE *vfp = NULL; 
  size_t file_size = 0; // number of bytes in file 
  char *X = NULL; size_t nX = 0;
  if ( !v->is_eov ) { go_BYE(-1); }
  WHOLE_VEC_REC_TYPE *w = 
    ptr_S->whole_vec_dir->whole_vecs + v->whole_vec_dir_idx;
  if ( w->uqid != v->uqid ) { go_BYE(-1); } 
  // Nothing to do: file exists 
  if ( w->is_file ) { return status; } 

  status = mk_file_name(ptr_S, v->uqid, &vfile_name); cBYE(status);
  vfp = fopen(vfile_name, "wb"); 
  return_if_fopen_failed(vfp, vfile_name, "wb"); 
  size_t chnk_sz = get_chunk_size_in_bytes(ptr_S, v);
  for ( unsigned int i = 0; i < v->num_chunks; i++ ) { 
    int nw;
    uint32_t chunk_idx = v->chunks[i];
    chk_chunk_idx(chunk_idx);
    CHUNK_REC_TYPE *ptr_chunk = ptr_S->chunk_dir[i]->chunks + chunk_idx;
    if ( ptr_chunk->data ) { 
      nw = fwrite(ptr_chunk->data, chnk_sz, 1, vfp);
      if ( nw != 1 ) { go_BYE(-1); }
    }
    else {
      char *cfile_name = NULL;
      status = mk_file_name(ptr_S, ptr_chunk->uqid, &cfile_name);
      cBYE(status);
      if ( nX != chnk_sz ) { go_BYE(-1); }
      status = rs_mmap(cfile_name, &X, &nX, 0); cBYE(status);
      nw = fwrite(X, nX, 1, vfp);
      if ( nw != 1 ) { go_BYE(-1); }
      munmap(X, nX); X = NULL; nX = 0;
      free_if_non_null(cfile_name);
    }
    file_size += chnk_sz;
  }
  w->is_file = true;
  w->file_size = file_size;
BYE:
  free_if_non_null(vfile_name);
  fclose_if_non_null(vfp);
  return status;
}

int
qmem_load_chunk(
    const CHUNK_REC_TYPE *const chunk, 
    const VEC_REC_TYPE *const ptr_vec,
    char **ptr_data
    )
{
  int status = 0;
  char *data = NULL;
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
