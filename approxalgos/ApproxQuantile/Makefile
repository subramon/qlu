#CC = icc -O3 -std=c99 
#LINKOPTIONS = -ipp -DIPP -DCILK -lm -g


INCS = -I./src/ -I./inc/ -I../../UTILS/inc/
all: libapprox_quantile.so

SRCS = \
	src/approx_quantile.c \
	src/Collapse.c \
	src/determine_b_k.c \
	src/fast_output.c \
	src/New.c \
	src/qsort_asc_F8.c \
	src/slow_output.c 


libapprox_quantile.so: ${SRCS}  ./inc/*.h
	gcc ${QC_FLAGS} ${INCS} ${SRCS} ../../UTILS/src/rdtsc.c \
	../../UTILS/src/spooky_hash.c \
	-shared -o libapprox_quantile.so

test_approx_quantile: $(SRCS) test/*.c libapprox_quantile.so
	gcc ${QC_FLAGS} ${INCS} test/test_approx_quantile.c \
	-o test_approx_quantile \
	./libapprox_quantile.so -lm 
	./test_approx_quantile

clean:
	rm -f *.o *.so test_approx_quantile
