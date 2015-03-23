SRC = ../APP/src/
INC = ../APP/inc/
INCS = -I../APP/inc
OBJS = 	main.o \
		uart_api.o \
		AisleManage.o \
		MyPublicFunction.o \
		MyClientSocket.o \
		MyServerSocket.o \
		MyCustMadeJson.o \
		RemoteCmds.o \
		AsyncCmdActions.o \
		AsyncCmds.o \
		xProtocol.o

HOST_CC = gcc
TARGET_CC = arm-linux-gcc
CC = ${TARGET_CC}

APP_NAME = Bake_Tobacco_Monitor

${APP_NAME}:${OBJS}
	${CC} -o ../OUT/${APP_NAME} $^ -lpthread
		
%.o:${SRC}%.c
	${CC} -c ${INCS} $< -o $@	

.PHONY : clean
clean :
	-rm -f ${OBJS} ../OUT/${APP_NAME}
