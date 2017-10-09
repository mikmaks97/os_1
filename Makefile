########################################
##
## Makefile
## LINUX compilation
##
##############################################

#FLAGS
C++FLAG = -g -std=c++11

MATH_LIBS = -lm
TEMP_DIR=~/temp
EXEC_DIR=$(TEMP_DIR)


.cpp.o:
	g++ $(C++FLAG) $(INCLUDES)  -c $< -o $@


#Including
INCLUDES=  -I.

#-->All libraries (without LEDA)
LIBS_ALL =  -L/usr/lib -L/usr/local/lib $(MATH_LIBS)


#Gray to binary program
ALL_OBJ1=run_os.o os.o
PROGRAM_1=run.me
$(PROGRAM_1): $(ALL_OBJ1)
	mkdir $(TEMP_DIR)
	g++ $(C++FLAG) -o $(EXEC_DIR)/$@ $(ALL_OBJ1) $(INCLUDES) $(LIBS_ALL)


all:
	make $(PROGRAM_1)

.PHONY: clean
clean:
	(rm -f *.o;)

(:
