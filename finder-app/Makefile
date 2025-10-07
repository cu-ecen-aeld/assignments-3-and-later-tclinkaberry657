# Make file for writer.c

CC=${CROSS_COMPILE}gcc
TARGET=writer

CFILES= writer.c

SRCS= ${CFILES}
OBJS= ${CFILES:.c=.o}

all: ${TARGET}

${TARGET}: ${OBJS}
	${CC} ${OBJS} -o $@

%.o: %.c
	${CC} -c $< -o $@

clean:
	-rm -f ${OBJS} ${TARGET}

