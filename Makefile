########################################
# Read version file first line.
version=$(shell head -1 Version)

#########################################
# Open module -> 1
# Close module -> 0
mqttlink=1
##########################################


params=
libs=
build_mqttlink=


ifeq ($(mqttlink),1)
params+= -DMQTTLINK
libs+=-lmosquitto
build_mqttlink=libmqttlink.o
include_h+=./include/libmqttlink.h
endif


OS := $(shell uname)

ifeq ($(OS), Linux)
params+= -DOS_Linux
endif

ifeq ($(OS), Windows)
params+= -DOS_Windows
endif

all: mqttlink.so

mqttlink.so: $(build_mqttlink)
	gcc -shared -o libmqttlink.so $(build_mqttlink) $(libs) $(params) -lpthread 
	strip --strip-unneeded libmqttlink.so


libmqttlink.o: src/libmqttlink.c include/libmqttlink.h
	gcc -O3 -Wall -fpic -c src/libmqttlink.c $(params)

# Removed standalone utility compilation
# libmqttlink_utility_functions.o: src/libmqttlink_utility_functions.c include/libmqttlink_utility_functions.h
# 	g++ -O3 -Wall -fpic -c src/libmqttlink_utility_functions.c $(params)


install: 
	@rm -rf /usr/local/include/libmqttlink*
	@mkdir -p /usr/local/include/libmqttlink
	
	@cp $(include_h) /usr/local/include/libmqttlink
# utility header removed

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

