ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))/install/lib

CC=gcc
CFLAGS=-Wall -Werror -I include/ -g -O0
SRCDIR=src
TESTDIR=test
INSTALLLIBDIR=install/lib
INSTALLBINDIR=install/bin
GRAPHFILEDIR=graphs/files
GRAPHIMGDIR=graphs/imgs
GRAPHTESTDIR=graphs/Tests

TESTSRCS=$(wildcard $(TESTDIR)/*.c)
TESTEXES=$(patsubst %.c,%,$(TESTSRCS))
GRAPHSRCS=$(wildcard $(GRAPHTESTDIR)/*.c)
GRAPHLIST=$(patsubst %.c,%,$(notdir $(GRAPHSRCS)))

.PHONY: all check valgrind pthreads graphs install clean

all: lib tests install

lib: $(INSTALLLIBDIR)/libthread.so

tests: PFLAGS=-L $(INSTALLLIBDIR) -lthread
tests: $(TESTEXES)

pthreads: PFLAGS=-DUSE_PTHREAD -pthread
pthreads: $(TESTEXES) install

$(INSTALLLIBDIR)/libthread.so: src/thread.c src/mutex.c src/utils.c include/thread.h
	$(CC) $(CFLAGS) -c -fPIC src/thread.c -o src/thread.o
	$(CC) $(CFLAGS) -c -fPIC src/mutex.c -o src/mutex.o
	$(CC) $(CFLAGS) -c -fPIC src/utils.c -o src/utils.o
	$(CC) $(CFLAGS) src/thread.o src/mutex.o src/utils.o -shared -o $(INSTALLLIBDIR)/libthread.so

test/%: test/%.c $(INSTALLLIBDIR)/libthread.so
	$(CC) $(CFLAGS) -o $@ $< $(PFLAGS)

install:
	@for x in $(TESTEXES) ; do \
        if [ -f $$x ]; then\
        	mv $$x $(INSTALLBINDIR) ; \
        fi \
    done

check:
	LD_LIBRARY_PATH="$(ROOT_DIR)" ./install/bin/01-main
	LD_LIBRARY_PATH="$(ROOT_DIR)" ./install/bin/02-switch
	LD_LIBRARY_PATH="$(ROOT_DIR)" ./install/bin/11-join
	LD_LIBRARY_PATH="$(ROOT_DIR)" ./install/bin/12-join-main
	LD_LIBRARY_PATH="$(ROOT_DIR)" ./install/bin/21-create-many 20
	LD_LIBRARY_PATH="$(ROOT_DIR)" ./install/bin/22-create-many-recursive 20
	LD_LIBRARY_PATH="$(ROOT_DIR)" ./install/bin/23-create-many-once 20
	LD_LIBRARY_PATH="$(ROOT_DIR)" ./install/bin/31-switch-many 10 20
	LD_LIBRARY_PATH="$(ROOT_DIR)" ./install/bin/32-switch-many-join 10 20
	LD_LIBRARY_PATH="$(ROOT_DIR)" ./install/bin/33-switch-many-cascade 10 20
	LD_LIBRARY_PATH="$(ROOT_DIR)" ./install/bin/51-fibonacci 8
	LD_LIBRARY_PATH="$(ROOT_DIR)" ./install/bin/61-mutex 20
	LD_LIBRARY_PATH="$(ROOT_DIR)" ./install/bin/62-mutex 20
	LD_LIBRARY_PATH="$(ROOT_DIR)" ./install/bin/63-big-sum 10000
	LD_LIBRARY_PATH="$(ROOT_DIR)" ./install/bin/example
	LD_LIBRARY_PATH="$(ROOT_DIR)" ./install/bin/search

valgrind:
	LD_LIBRARY_PATH="$(ROOT_DIR)" valgrind ./install/bin/01-main
	LD_LIBRARY_PATH="$(ROOT_DIR)" valgrind ./install/bin/02-switch
	LD_LIBRARY_PATH="$(ROOT_DIR)" valgrind ./install/bin/11-join
	LD_LIBRARY_PATH="$(ROOT_DIR)" valgrind ./install/bin/12-join-main
	LD_LIBRARY_PATH="$(ROOT_DIR)" valgrind ./install/bin/21-create-many 20
	LD_LIBRARY_PATH="$(ROOT_DIR)" valgrind ./install/bin/22-create-many-recursive 20
	LD_LIBRARY_PATH="$(ROOT_DIR)" valgrind ./install/bin/23-create-many-once 20
	LD_LIBRARY_PATH="$(ROOT_DIR)" valgrind ./install/bin/31-switch-many 10 20
	LD_LIBRARY_PATH="$(ROOT_DIR)" valgrind ./install/bin/32-switch-many-join 10 20
	LD_LIBRARY_PATH="$(ROOT_DIR)" valgrind ./install/bin/33-switch-many-cascade 10 20
	LD_LIBRARY_PATH="$(ROOT_DIR)" valgrind ./install/bin/51-fibonacci 8
	LD_LIBRARY_PATH="$(ROOT_DIR)" valgrind ./install/bin/61-mutex 20
	LD_LIBRARY_PATH="$(ROOT_DIR)" valgrind ./install/bin/62-mutex 20
	LD_LIBRARY_PATH="$(ROOT_DIR)" valgrind ./install/bin/63-big-sum 10000
	LD_LIBRARY_PATH="$(ROOT_DIR)" valgrind ./install/bin/example
	LD_LIBRARY_PATH="$(ROOT_DIR)" valgrind ./install/bin/search

graphs:
	@if [ ! -d $(GRAPHFILEDIR) ]; then \
		mkdir $(GRAPHFILEDIR); \
		mkdir $(GRAPHIMGDIR); \
	fi; \
	for x in $(GRAPHLIST); do \
		echo "Plotting "$$x; \
		$(CC) $(CFLAGS) -o $(GRAPHFILEDIR)/$$x $(GRAPHTESTDIR)/$$x.c -L $(INSTALLLIBDIR) -lthread; \
		LD_LIBRARY_PATH="$(ROOT_DIR)" $(GRAPHFILEDIR)/$$x $(GRAPHFILEDIR)/$$x.dat $(GRAPHFILEDIR)/$$x; \
		$(CC) $(CFLAGS) -o $(GRAPHFILEDIR)/$$x"_pthread" $(GRAPHTESTDIR)/$$x.c -L $(INSTALLLIBDIR) -DUSE_PTHREAD -pthread; \
		touch $(GRAPHFILEDIR)/$$x"_pthread.dat"; \
		LD_LIBRARY_PATH="$(ROOT_DIR)" $(GRAPHFILEDIR)/$$x"_pthread" $(GRAPHFILEDIR)/$$x"_pthread.dat" $(GRAPHFILEDIR)/$$x"_pthread"; \
		gnuplot -c ./test/plotting"_"$$x.gpi $(GRAPHFILEDIR)/$$x $$x; \
	 done

plotting:
	@for x in $(GRAPHLIST); do \
		gnuplot -c ./test/plotting"_"$$x.gpi $(GRAPHFILEDIR)/$$x $$x; \
	 done

test_preemption:
	 	 	@if [ ! -d $(GRAPHFILEDIR) ]; then \
	 	 		mkdir $(GRAPHFILEDIR); \
	 	 		mkdir $(GRAPHIMGDIR); \
	 	 	fi; \
	 	 	if [ -f $(GRAPHFILEDIR)/"preemption_big-sum.dat" ]; then \
	 	 		rm -f $(GRAPHFILEDIR)/"preemption_big-sum.dat"; \
	 	 	fi; \
	 	 	if [ -f $(GRAPHFILEDIR)/"preemption_search.dat" ]; then \
	 	 		rm -f $(GRAPHFILEDIR)/"preemption_search.dat"; \
	 	 	fi; \
	 	 	touch $(GRAPHFILEDIR)/"preemption_big-sum.dat"; \
	 	 	$(CC) $(CFLAGS) -o $(GRAPHFILEDIR)/63-big-sum $(GRAPHTESTDIR)/63-big-sum.c -L $(INSTALLLIBDIR) -lthread; \
	 	 	INDEX=100; \
  	 	 	echo "Test big sum begin";\
			while [ $$INDEX -le 50000 ] ; do \
	 	 	LD_LIBRARY_PATH="$(ROOT_DIR)" $(GRAPHFILEDIR)/63-big-sum $(-DTIMESLICE=$$index) $(GRAPHFILEDIR)/"preemption_big-sum.dat" $$INDEX 0; \
	 	 	INDEX=`expr $$INDEX + 500` ;\
	 	 	echo $$INDEX; \
	 	 	done; \
	 	 	LD_LIBRARY_PATH="$(ROOT_DIR)" $(GRAPHFILEDIR)/63-big-sum $(-DUSE_PREEMPTIVE=0) $(GRAPHFILEDIR)/"preemption_big-sum.dat" 50500 0; \
	 	 	gnuplot -c ./test/plotting_preemption-big-sum.gpi $(GRAPHFILEDIR)/"preemption_big-sum" "preemption_big-sum"; \
			echo "Test big sum done";\
  	 	 	echo "Test search begin";\
	 	 	touch $(GRAPHFILEDIR)/"preemption_search.dat"; \
	 	 	$(CC) $(CFLAGS) -o $(GRAPHFILEDIR)/preemption_search $(GRAPHTESTDIR)/search.c -L $(INSTALLLIBDIR) -lthread; \
	 	 	INDEX=50; \
	 	 	while [ $$INDEX -le 1000 ] ; do \
	 	 		LD_LIBRARY_PATH="$(ROOT_DIR)" $(GRAPHFILEDIR)/preemption_search $(GRAPHFILEDIR)/"preemption_search.dat" $$INDEX; \
	 	 		INDEX=`expr $$INDEX + 50` ;\
	 	 		echo $$INDEX;\
	 	 	done; \
	 	 	LD_LIBRARY_PATH="$(ROOT_DIR)" $(GRAPHFILEDIR)/preemption_search $(-DUSE_PREEMPTIVE=0) $(GRAPHFILEDIR)/"preemption_search.dat" 20050; \
	 	 	gnuplot -c ./test/plotting_preemption-search.gpi $(GRAPHFILEDIR)/"preemption_search" "preemption_search";

clean:
	rm -rf ./vgcore* ./$(INSTALLLIBDIR)/* ./$(INSTALLBINDIR)/* ./$(SRCDIR)/*.~ ./$(SRCDIR)/*.o ./$(GRAPHIMGDIR) ./$(GRAPHFILEDIR)
