#CROSS_COMPILE=aarch64-none-linux-gnu-
CC = ${CROSS_COMPILE}gcc

CFLAGS = -Wall -Werror -g -O0
LDFLAGS = -pthread

TARGET = aesdsocket
CFILES = aesdsocket.c

SRCS = ${CFILES}
OBJS = ${CFILES:.c=.o}

all: ${TARGET}

${TARGET}: ${SOURCE}
	${CC} ${CFLAGS} ${SRCS} -o ${TARGET} ${LDFLAGS}

clean:
	rm -f ${OBJS} ${TARGET}

default: all

