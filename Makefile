# Got inspiration from the link:
# https://www.geeksforgeeks.org/cpp/makefile-in-c-and-its-applications/

# Compiler
CXX = g++

# Compiler flags: -Wall enables all common warnings, -g adds debugging info, -std=c++17 specifies the C++ standard
CXXFLAGS = -Wall -g -std=c++17 -Iinclude

# Executable name
TARGET = main

# Source files
SRCS = src/main.cpp

# Object files (stored in objectFiles/)
OBJDIR = objectFiles
OBJS = $(SRCS:src/%.cpp=$(OBJDIR)/%.o)

# Default rule to build and run the executable (direct compilation)
all: $(OBJDIR) $(TARGET)-obj
	./$(TARGET)

# Rule to link object files into the executable
$(TARGET)-obj: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Rule to create objectFiles directory
$(OBJDIR):
	@mkdir -p $(OBJDIR)

# Rule to compile .cpp files into .o files in objectFiles/
$(OBJDIR)/%.o: src/%.cpp include/main.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule to remove generated files
clean:
	rm -f $(OBJDIR)/* ./$(TARGET)

# Phony targets (not actual files)
.PHONY: all clean
