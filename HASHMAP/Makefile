
#CC = icc -O3 -std=c99 
#LINKOPTIONS = -ipp -DIPP -DCILK -lm -g


INCS = -I ./src/ -I../UTILS/inc/ -I./inc/
all: hmap.so test1 test2

V1SRCS = \
	src/val_update_1.c \
	src/val_free_1.c 

V2SRCS = \
	src/val_update_2.c \
	src/val_free_2.c 

SRCS = \
	  ../UTILS/src/get_time_usec.c  \
	  ../UTILS/src/spooky_hash.c  \
	  ../UTILS/src/rdtsc.c  \
	  ./src/hmap_aux.c  \
	  ./src/hmap_chk.c  \
	  ./src/hmap_del.c  \
	  ./src/hmap_destroy.c  \
	  ./src/hmap_get.c  \
	  ./src/hmap_insert.c  \
	  ./src/hmap_instantiate.c  \
	  ./src/hmap_mput.c  \
	  ./src/hmap_put.c  \
	  ./src/hmap_resize.c  \
	  ./src/calc_new_size.c  \
	  ./src/murmurhash.c  \

.c.o : 
	$(CC) -c -o $@ $< $(QC_FLAGS)  $(INCS) 

libval1.so: ${V1SRCS}  ./inc/*.h 
	gcc -DCASEA ${QC_FLAGS} ${V1SRCS} ${INCS} \
	  -shared -o libval1.so 

libval2.so: ${V2SRCS}  ./inc/*.h 
	gcc -DCASEB ${QC_FLAGS} ${V2SRCS} ${INCS} \
	  -shared -o libval2.so 

hmap.so: ${SRCS}  ./inc/*.h 
	gcc -DCASEA ${QC_FLAGS} ${SRCS} ${INCS} -shared -o hmap.so 

test1 : hmap.so ./test/test1.c ${V1SRCS} libval1.so 
	gcc -DCASEA ${QC_FLAGS} ${INCS} ./test/test1.c \
	hmap.so libval1.so -o test1 

test2 : hmap.so ./test/test2.c ${V1SRCS}  libval1.so 
	gcc -DCASEA ${QC_FLAGS} ${INCS} ./test/test2.c \
	hmap.so libval1.so -o test2  -lm

test3 : hmap.so ./test/test3.c ${V2SRCS}  libval2.so 
	gcc -DCASEB ${QC_FLAGS} ${INCS} ./test/test3.c \
	hmap.so libval2.so -o test3  -lm

clean:
	rm -f *.o test_hmap *.so