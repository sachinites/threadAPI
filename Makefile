CC=gcc
CFLAGS=-g -Wall
LIBS=-lpthread 
OBJ=threadApi.o Queue.o
SLIB=threadlib.so
STATICLIB=threadlib.a
TARGET:${SLIB} ${STATICLIB} exe
%.o:%.c
	${CC} ${CFLAGS} -c -I . $<
${SLIB}: ${OBJ}
	${CC} ${CFLAGS} ${OBJ} -shared -o ${SLIB}
${STATICLIB}: ${OBJ}
	ar rs ${STATICLIB} ${OBJ} 
exe:main.o
	${CC} ${CFLAGS} main.o -o exe -L . ${STATICLIB}
clean:
	rm *.o
	rm ${SLIB} ${STATICLIB}
	rm exe
