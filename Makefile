CC=gcc
CFLAGS=-g -Wall
LIBS=-lpthread 
OBJ=threadApi.o
SLIB=threadlib.so
STATICLIB=threadlib.a
TARGET:${SLIB} ${STATICLIB} exe
%.o:%.c
	${CC} ${CFLAGS} -c -I . $<
${SLIB}: ${OBJ}
	${CC} ${CFLAGS} ${OBJ} -shared -o ${SLIB} ${LIBS}
${STATICLIB}: ${OBJ}
	ar rs ${STATICLIB} ${OBJ}
exe:main.o ${OBJ}
	${CC} ${CFLAGS} main.o ${OBJ} -o exe
clean:
	rm *.o
	rm ${SLIB} ${STATICLIB}
	rm exe
