ARM_LINUX_COMPILE := arm-xilinx-linux-gnueabi-
GCC := $(ARM_LINUX_COMPILE)gcc
GXX := $(ARM_LINUX_COMPILE)g++
STRIP := $(ARM_LINUX_COMPILE)strip

BIN := pgClient

COMP_FLAGS = -pipe -Wall -W -fPIC -Wl,-z,origin -Wl,-rpath,\$$ORIGIN -g -D_DEBUG

SUBDIRS=$(shell ls -l | grep ^d | awk '{if($$9 != "debug") print $$9}')

ROOT_DIR=$(shell pwd)
OBJS_DIR=debug/obj
BIN_DIR=debug/bin

LD_FLAGS = -L $(ROOT_DIR)/lib/ -lsqlite3 -lrt -lpthread
ALL_DIRS = $(shell find $(ROOT_DIR) -maxdepth 3 -type d)
INCLUDEPATH = $(shell find $(ROOT_DIR) -name "*.h" | awk '{print $$1}')
INCLUDEPATH := $(foreach n, $(INCLUDEPATH), $(dir $n))
INCLUDEPATH := $(foreach n, $(ALL_DIRS), $(if $(findstring $n, $(INCLUDEPATH)),-I$(n),))
INCLUDEPATH := $(strip $(INCLUDEPATH))

CUR_SOURCE_C=${wildcard *.c}
CUR_OBJS_C =${patsubst %.c, %.o, $(CUR_SOURCE_C)} 

CUR_SOURCE_CPP=${wildcard *.cpp}
CUR_OBJS_CPP =${patsubst %.cpp, %.o, $(CUR_SOURCE_CPP)} 

C_FILES := $(shell find $(ROOT_DIR) -name "*.c" | awk '{print $$1}')
CPP_FILES := $(shell find $(ROOT_DIR) -name "*.cpp" | awk '{print $$1}')

OBJS := ${patsubst %.c, %.o, $(C_FILES)} 
OBJS += ${patsubst %.cpp, %.o, $(CPP_FILES)} 

export GCC GXX LD_FLAGS COMP_FLAGS INCLUDEPATH

all:$(CUR_OBJS_C) $(CUR_OBJS_CPP)

	@for name in $(SUBDIRS); \
	do \
		make -C $$name || exit "$$?";\
	done
	
	$(GXX) -o $(BIN) $(OBJS) $(LD_FLAGS)
	$(STRIP) $(BIN) -o $(BIN)
    
$(CUR_OBJS_CPP):%.o:%.cpp
	$(GXX) $(COMP_FLAGS) -c -o $@ $< $(INCLUDEPATH)
	
$(CUR_OBJS_C):%.o:%.c
	$(GCC) $(COMP_FLAGS) -c -o $@ $< $(INCLUDEPATH)
 
clean:
	rm -rf $(BIN)
	rm -rf *.o
	@for name in $(SUBDIRS); \
	do \
		make clean -C $$name;\
	done
    
install:
	cp -rdf $(BIN) /tftpboot/
