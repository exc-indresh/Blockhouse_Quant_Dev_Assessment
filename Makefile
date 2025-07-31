CXX := g++
CXXFLAGS := -O3 -std=c++17

TARGET := reconstruction
SOURCES := dev.cpp

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
