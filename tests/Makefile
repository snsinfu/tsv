CXXFLAGS += \
  -std=c++17 \
  -Wpedantic \
  -Wall \
  -Wextra \
  -Wconversion \
  -Wsign-conversion \
  -g \
  -fsanitize=address \
  -isystem include \
  -I ../include

OBJECTS = \
  main.o \
  test_tsv.o \
  test_reflection.o \
  test_conversion.o \
  test_line_reader.o \
  test_parser.o


.PHONY: run clean
.SUFFIXES: .cc

run: main
	@./main

clean:
	rm -f main $(OBJECTS)

main: $(OBJECTS) Makefile
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $(OBJECTS)

.cc.o:
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

$(OBJECTS): ../include/tsv.hpp Makefile
