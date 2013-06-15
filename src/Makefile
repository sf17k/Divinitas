OBJECTS = main.o nbt.o export.o generate.o lithosphere.o plate.o sqrdmd.o
EXECUTABLE = ../divinitas.exe

CC = gcc
CCFLAGS = -O3 -Wall
CXX = g++
CXXFLAGS = -O3 -Wall -std=c++11 -g
LDFLAGS = -static-libgcc -static-libstdc++ -lopengl32 -lglu32 -lfreeglut -lnbt -lz -lboost_filesystem -lboost_system
OUT_DIR = ../bin
OUT_OBJS = $(addprefix $(OUT_DIR)/,$(OBJECTS))


all: divinitas

divinitas: $(EXECUTABLE)

$(EXECUTABLE): $(OUT_OBJS)
	$(CXX) $(OUT_OBJS) $(CXXFLAGS) $(LDFLAGS) -o $@

$(OUT_DIR)/%.o : %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(OUT_DIR)/%.o : %.c
	$(CC) -c $(CCFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f $(OUT_DIR)/*.o
	rm -f $(EXECUTABLE)

