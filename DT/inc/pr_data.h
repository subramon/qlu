extern int 
pr_data_f(
    float **X, /* [m][n] */
    uint32_t m,
    uint32_t n,
    uint8_t *g,
    uint32_t lb,
    uint32_t ub
   );
extern int 
pr_data_i(
    uint32_t **X, /* [m][n] */
    uint32_t **to, /* [m][n] */
    uint32_t m,
    uint32_t n,
    uint8_t *g,
    uint32_t lb,
    uint32_t ub
   );
