CC = ${CROSS_COMPILE}gcc

CFLAGS = -Wall -Werror -g -O0
LDFLAGS = -pthread

TARGET = aesdsocket
CFILES = aesdsocket.c

SRCS = ${CFILES}
OBJS = ${CFILES:.c=.o}

all: ${TARGET}

${TARGET}: ${SOURCE}
	${CC} ${CFLAGS} -o ${TARGET} ${SRCS} ${LDFLAGS}

clean:
	rm -f ${OBJS} ${TARGET}

default: all

