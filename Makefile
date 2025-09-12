########################################
# Read version file first line.
version=$(shell head -1 Version)

#########################################
# Open module -> 1
# Close module -> 0
mqttlink_mosq=1
##########################################


params=
libs=
build_mqttlink_mosq=


ifeq ($(mqttlink_mosq),1)
params+= -DMQTTLINK_MOSQ
libs+=-lmosquitto
build_mqttlink_mosq=libmqttlink_mosq.o
include_h+=./include/libmqttlink_mosq.h
endif


OS := $(shell uname)

ifeq ($(OS), Linux)
params+= -DOS_Linux
endif

ifeq ($(OS), Windows)
params+= -DOS_Windows
endif

all: mqttlink.so

mqttlink.so: $(build_mqttlink_mosq) libmqttlink_utility_functions.o
	g++ -shared -o libmqttlink.so $(build_mqttlink_mosq) libmqttlink_utility_functions.o $(libs) $(params) -lpthread 
	strip --strip-unneeded libmqttlink.so


libmqttlink_mosq.o: src/libmqttlink_mosq.c include/libmqttlink_mosq.h
	gcc -O3 -Wall -fpic -c src/libmqttlink_mosq.c $(params)

libmqttlink_utility_functions.o: src/libmqttlink_utility_functions.cpp include/libmqttlink_utility_functions.h
	g++ -O3 -Wall -fpic -c src/libmqttlink_utility_functions.cpp $(params)


install: 
	@rm -rf /usr/local/include/libmqttlink*
	@mkdir -p /usr/local/include/libmqttlink
	
	@cp include/libmqttlink_utility_functions.h  $(include_h) /usr/local/include/libmqttlink

	@rm -f /usr/local/lib/libmqttlink.so*
	@cp libmqttlink.so /usr/local/lib/libmqttlink.so.$(version)
	@cd /usr/local/lib/ ; ln -s libmqttlink.so.$(version) libmqttlink.so
 
	@rm -f ./libmqttlink.pc
	@rm -f /usr/local/lib/pkgconfig/libmqttlink.pc

	@echo "Name: libmqttlink" >> libmqttlink.pc
	@echo "Description: libmqttlink" >> libmqttlink.pc
	@echo "Version: $(version)" >> libmqttlink.pc
	@echo "Libs: -L/usr/local/lib -lmqttlink" >> libmqttlink.pc
	@echo "Cflags: -I/usr/local/include/libmqttlink" >> libmqttlink.pc

	@cp libmqttlink.pc /usr/local/lib/pkgconfig
	@ldconfig

uninstall:
	rm -f /usr/local/lib/libmqttlink.so*
	rm -rf /usr/local/include/libmqttlink*
	rm -f /usr/local/lib/pkgconfig/libmqttlink.pc

clean:
	rm -f *.so *.pc *.o

