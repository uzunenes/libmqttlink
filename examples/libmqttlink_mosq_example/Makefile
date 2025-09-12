PROJECT = test_mqttlink_mosq
CC = gcc
CPP = g++
CFLAGS = -Wall -O3

OBJCC = $(patsubst src/%.c, obj/%.o, $(wildcard src/*c))
OBJCPP = $(patsubst src/%.cpp, obj/%.o, $(wildcard src/*cpp))

LDFLAGS = -lmqttlink

all: obj $(PROJECT)

$(PROJECT): $(OBJCC) $(OBJCPP)
	$(CPP) $(OBJCC) $(OBJCPP) $(LDFLAGS) -o $(PROJECT)

obj/%.o: src/%.c
	$(CC) -c $< $(CFLAGS) -o $@

obj/%.o: src/%.cpp
	$(CPP) -c $< $(CFLAGS) -o $@


obj:
	mkdir -p obj

clean:
	rm -f $(PROJECT) obj/*.o