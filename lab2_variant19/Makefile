CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2
LDFLAGS = -lm
TARGET = kmeans
SRCS = main.c kmeans_serial.c kmeans_parallel.c

all: $(TARGET)

$(TARGET): $(SRCS) kmeans.h
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)
