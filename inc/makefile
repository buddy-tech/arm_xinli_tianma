SUBDIRS=$(shell ls -l | grep ^d | awk '{print $$9}') 
CUR_SOURCE_C=${wildcard *.c}  
CUR_OBJS_C =${patsubst %.c, %.o, $(CUR_SOURCE_C)} 
 
CUR_SOURCE_CPP=${wildcard *.cpp}  
CUR_OBJS_CPP =${patsubst %.cpp, %.o, $(CUR_SOURCE_CPP)} 

all:$(CUR_OBJS_C) $(CUR_OBJS_CPP)
	@for name in $(SUBDIRS); \
	do \
		make -C $$name || exit "$$?";\
	done
	
$(CUR_OBJS_CPP):%.o:%.cpp
	$(GXX) $(COMP_FLAGS) -c -o $@ $< $(INCLUDEPATH)
	
$(CUR_OBJS_C):%.o:%.c
	$(GCC) $(COMP_FLAGS) -c -o $@ $< $(INCLUDEPATH)
	
clean:
	rm -rf *.o
	@for name in $(SUBDIRS); \
	do \
		make clean -C $$name;\
	done
	