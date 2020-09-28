#undef DEVELOPMENT
#ifdef DEVELOPMENT
#define MIN_LEAF_SIZE 32
#define NUM_FEATURES  4
#define NUM_INSTANCES 1024
#define BUFSZ  8
#define MIN_PARTITION_SIZE MIN_LEAF_SIZE
#define MAX_NUM_NODES_DT (NUM_INSTANCES / MIN_LEAF_SIZE)
#endif

#define PERF_TEST
#ifdef PERF_TEST
#define MIN_LEAF_SIZE 64
#define NUM_FEATURES  16
#define NUM_INSTANCES 65536
#define BUFSZ         64
// #define MIN_LEAF_SIZE 64
// #define NUM_FEATURES  32
// #define NUM_INSTANCES 524288
// #define BUFSZ         1024
#define MIN_PARTITION_SIZE MIN_LEAF_SIZE
#define MAX_NUM_NODES_DT (NUM_INSTANCES / MIN_LEAF_SIZE)
#endif
