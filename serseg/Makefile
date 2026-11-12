CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
SOURCES = compiler/main.cpp compiler/modules.cpp compiler/optimizer.cpp compiler/lexer.cpp compiler/parser.cpp compiler/semantic.cpp compiler/backend_x86_64.cpp
ifeq ($(OS),Windows_NT)
BIN = build/serseg.exe
OPTTEST = build/optimizer-test.exe
QUOTE_TEST = build/windows-quote-test.exe
WINPROCTEST = build/windows-process-test.exe
PLATFORM_LIBS = -lshell32
else
BIN = build/serseg
OPTTEST = build/optimizer-test
QUOTE_TEST = build/windows-quote-test
PLATFORM_LIBS =
endif
.PHONY: all test clean
all: $(BIN)
$(BIN): $(SOURCES) compiler/serseg.hpp compiler/runtime.hpp compiler/windows_quote.hpp compiler/windows_process.hpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $@ $(PLATFORM_LIBS)
$(OPTTEST): tests/optimizer.cpp compiler/optimizer.cpp compiler/lexer.cpp compiler/parser.cpp compiler/semantic.cpp compiler/backend_x86_64.cpp compiler/serseg.hpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(filter %.cpp,$^) -o $@
$(QUOTE_TEST): tests/windows-quote.cpp compiler/windows_quote.hpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) tests/windows-quote.cpp -o $@
ifeq ($(OS),Windows_NT)
$(WINPROCTEST): tests/windows-process.cpp compiler/windows_process.hpp compiler/windows_quote.hpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) tests/windows-process.cpp -o $@ -lshell32
endif
test: $(BIN) $(OPTTEST) $(QUOTE_TEST) $(WINPROCTEST)
	"$(abspath $(OPTTEST))"
	"$(abspath $(QUOTE_TEST))"
	bash tests/integration.sh "$(abspath $(BIN))"
	bash tests/encoding.sh "$(abspath $(BIN))"
	bash tests/modules-qualified.sh "$(abspath $(BIN))"
	bash tests/stdlib.sh "$(abspath $(BIN))"
	bash tests/managed-text.sh "$(abspath $(BIN))"
	bash tests/text-builtins.sh "$(abspath $(BIN))"
	bash tests/text-ops.sh "$(abspath $(BIN))"
	bash tests/conditions-and-validation.sh "$(abspath $(BIN))"
	bash tests/managed-arrays.sh "$(abspath $(BIN))"
	bash tests/typed-arrays.sh "$(abspath $(BIN))"
	bash tests/iteration.sh "$(abspath $(BIN))"
	bash tests/input.sh "$(abspath $(BIN))"
	python3 tests/expressions.py "$(abspath $(BIN))"
	python3 tests/expressions.py "$(abspath $(BIN))" -O1
	python3 tests/floats.py "$(abspath $(BIN))"
	python3 tests/vscode.py
	python3 tests/utf8.py "$(abspath $(BIN))"
ifeq ($(OS),Windows_NT)
	"$(abspath $(WINPROCTEST))"
	python3 tests/windows.py "$(abspath $(BIN))"
else
	bash tests/install-linux.sh
endif
clean:
	rm -rf build
