

headers := $(wildcard *.h)

all : test.exe

%.o : %.c ${headers}
	${CC} -o $@ -c $< -g -O2 -Wall

test.exe : test.o snprintfs.o
	${CC} -o $@ $^

clean:
	rm -f *.o
	rm -f *.exe
