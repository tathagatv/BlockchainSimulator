CC=g++
CFLAGS=-g -Wall

FILES=block blockchain event link main peer simulator transaction
H_FILES=headers.h declarations.h trace.h
O_FILES=$(FILES:=.o)
TARGET=blockchain_simulator

%.o: %.cpp %(H_FILES)
	$(CC) -c $(CFLAGS) $< -o $@

$(TARGET): $(O_FILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(O_FILES)

clean:
	rm -f $(TARGET) *.o
