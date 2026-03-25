CC := clang

UNAME_S := $(shell uname -s)

# 按系统设置链接参数：Linux 加 -lrt，Mac 不加
ifeq ($(UNAME_S),Linux)
    LDFLAGS := -lrt
else ifeq ($(UNAME_S),Darwin)
    LDFLAGS := 
endif

ifeq ($(DEBUG),1)
	CFLAGS := -Wall -O0 -g
else
	CFLAGS := -Wall -O1 -DNDEBUG 
endif

all: isort sum

isort.o: isort.c 
	$(CC) $(CFLAGS) -c isort.c 

qsort.o: qsort.c 
	$(CC) $(CFLAGS) -c qsort.c 

isort: isort.o qsort.o
	$(CC) -o isort isort.o qsort.o $(LDFLAGS)

sum.o: sum.c 
	$(CC) $(CFLAGS) -c sum.c 

sum: sum.o
	$(CC) -o sum sum.o $(LDFLAGS)

clean:
	rm -f isort sum *.o *.gcda *.gcno *.gcov perf.data */perf.data cachegrind.out.*
