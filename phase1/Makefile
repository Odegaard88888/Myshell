CC = gcc
CFLAGS = -Og
LDLIBS = -lpthread
TARGET = myShell

myShell: myshell.o
	$(CC) $(CFLAGS) -o $(TARGET) myshell.o $(LDLIBS)

myshell.o : myshell.c
	$(CC) $(CFLAGS) -c -o myshell.o myshell.c
clean:
	rm -rf *~ $(TARGET)

