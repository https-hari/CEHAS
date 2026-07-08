CXX    = g++
CFLAGS = -Wall -Wextra -pthread -std=c++17
TARGET = cehas

SRCS = src/main.cpp \
       src/allocator.cpp \
       src/tracker.cpp \
       src/reporter.cpp \
       src/server.cpp

$(TARGET): $(SRCS)
	$(CXX) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) output/*.txt