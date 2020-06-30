OBJECTS = 20171101.o assemble.o linkload.o run.o

CC = gcc
TARGET = 20171101.out
CFLAGS = -W -Wall

$(TARGET) : $(OBJECTS)
	$(CC) -o $@ $(OBJECTS)

clean:
	rm $(OBJECTS) $(TARGET) 

