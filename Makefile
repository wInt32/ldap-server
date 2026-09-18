.PHONY: clean loc

SOURCES=$(shell find -type f -name *.cpp)
HEADERS=$(shell find -type f -name *.hpp)
OBJS=$(subst ./src,./obj,$(subst .cpp,.o,$(SOURCES)))

CXXFLAGS=-std=c++14 -pedantic -fanalyzer -Wall -Wextra -Wfatal-errors -g

#@if [ ! -d "obj/ber" ]; then mkdir -p obj/ber; fi 

all: isa-ldapserver

obj/%.o: src/%.cpp $(HEADERS)
	@if [ ! -d "obj/ldap" ]; then mkdir -p obj/ldap; fi 
	$(CXX) -c $< $(CXXFLAGS) -o $@

isa-ldapserver: $(OBJS)
	$(CXX) $(OBJS) -o isa-ldapserver

clean:
	rm -f ./isa-ldapserver
	rm -rf ./obj

loc:
	@echo Total lines of code: $$(find src -type f -name \*.cpp -or -name \*.hpp | xargs cat | wc -l)

check:
	cppcheck src/ --std=c++14 --enable=performance  --enable=portability --enable=warning