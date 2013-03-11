CC = cc
CXX = c++
CFLAGS = -O3 -DNDEBUG -fvisibility=hidden -fvisibility-inlines-hidden -I../../src
CXXFLAGS = -fno-rtti -fno-exceptions -stdlib=libc++ -std=c++11
OBJCFLAGS = -fobjc-arc
LDFLAGS = -stdlib=libc++ -framework Foundation -Wl,-S -Wl,-x -Wl,-dead_strip

OBJECTS := $(patsubst %.cc,%.o,$(wildcard ../../src/jscre/*.cc)) \
           $(patsubst %.mm,%.o,$(wildcard ../../src/*.mm)) \
           $(patsubst %.m,%.o,$(wildcard *.m))

%.o: %.cc
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c -o $@ $<

%.o: %.mm
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(OBJCFLAGS) -c -o $@ $<

%.o: %.m
	$(CC) $(CFLAGS) $(OBJCFLAGS) -c -o $@ $<

simple: $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(OBJECTS) simple

.PHONY: clean
