################OPTION###################
OPTION = -g -DNDEBUG
CCOMPILE = gcc $(OPTION) -m64
CPPCOMPILE = g++ $(OPTION) -m64
COMPILEOPTION =
INCLUDEDIR = -I/home/yiyang/zzy/redis/include/hiredis ae/
LINK = gcc $(OPTION) -m64
LINKOPTION = -o test
LIBDIRS = 
OBJS =  main.o redis_help.o
      
OUTPUT = test
SHAREDLIB = -ldl -pthread -L/home/yiyang/zzy/redis/lib -lhiredis 
APPENDLIB =  ae/ae.o ae/zmalloc.o ae/libjemalloc.a

################OPTION END################ae/ae.o ae/zmalloc.o

$(OUTPUT):$(OBJS) $(APPENDLIB)
	$(CPPCOMPILE)  $(LINKOPTION)  $(LIBDIRS)   $(OBJS) $(SHAREDLIB) $(APPENDLIB) 
	#rm -rf $(OBJS)
	#cp DpiUD ../

	
clean: 
	rm -rf $(OUTPUT)
	rm -f *.o
	

all: clean $(OUTPUT)
.PRECIOUS:%.cpp %.c %.C
.SUFFIXES:
.SUFFIXES:  .c .o .cpp .ecpp .pc .ec .C .cc .cxx .h .y

.cpp.o:
	$(CPPCOMPILE) -c -o $*.o $(COMPILEOPTION) $(INCLUDEDIR)  $*.cpp
	
.cc.o:
	$(CCOMPILE) -c -o $*.o $(COMPILEOPTION) $(INCLUDEDIR)  $*.cpp

.cxx.o:
	$(CPPCOMPILE) -c -o $*.o $(COMPILEOPTION) $(INCLUDEDIR)  $*.cpp

.c.o:
	$(CCOMPILE) -c -o $*.o $(COMPILEOPTION) $(INCLUDEDIR) $*.c

.C.o:
	$(CPPCOMPILE) -c -o $*.o $(COMPILEOPTION) $(INCLUDEDIR) $*.C	


 