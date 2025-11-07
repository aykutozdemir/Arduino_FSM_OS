<<<<<<< HEAD:Makefile
# Makefile for FsmOS Documentation and Compilation Testing
# FsmOS v1.3.0 - Standalone Library Documentation

.PHONY: all clean docs help test-compile test-fsmos test-examples test-libs test-all
=======
# Makefile for FsmOS Documentation
# FsmOS v1.3.0 - Standalone Library Documentation

.PHONY: all clean docs help
>>>>>>> 65bdf6ed5a850446de0e3034b2d80def63f1efd6:FsmOS/Makefile

# Default target
all: docs

# Generate documentation
docs:
	@echo "Generating FsmOS documentation..."
	doxygen Doxyfile
	@echo "Documentation generated in docs/html/"
	@echo "Open docs/html/index.html to view"

<<<<<<< HEAD:Makefile
# Test compilation of all examples
test-compile: test-fsmos test-examples test-libs
	@echo ""
	@echo "=========================================="
	@echo "All compilation tests completed!"
	@echo "=========================================="

# Test FsmOS library compilation
test-fsmos:
	@echo ""
	@echo "=========================================="
	@echo "Testing FsmOS Library Compilation"
	@echo "=========================================="
	@echo "Target: Arduino Uno (arduino:avr:uno)"
	@echo ""
	@echo -n "  FsmOS library... "; \
	if arduino-cli compile --fqbn arduino:avr:uno --library lib/FsmOS examples/BasicBlink/ >/dev/null 2>&1; then \
		echo "✓ PASS"; \
	else \
		echo "✗ FAIL"; \
		echo ""; \
		echo "FsmOS library compilation failed!"; \
		exit 1; \
	fi
	@echo ""
	@echo "FsmOS library compilation: ✓ PASSED"

# Test FsmOS examples compilation
test-examples:
	@echo ""
	@echo "=========================================="
	@echo "Testing FsmOS Examples Compilation"
	@echo "=========================================="
	@echo "Target: Arduino Uno (arduino:avr:uno)"
	@echo ""
	@success=0; \
	failed=0; \
	total=0; \
	for example in examples/*/; do \
		if [ -f "$$example"*.ino ]; then \
			sketch=$$(basename "$$example"); \
			total=$$((total+1)); \
			echo -n "  [$$total] $$sketch... "; \
			if arduino-cli compile --fqbn arduino:avr:uno --library lib/FsmOS "$$example" >/dev/null 2>&1; then \
				echo "✓ PASS"; \
				success=$$((success+1)); \
			else \
				echo "✗ FAIL"; \
				failed=$$((failed+1)); \
			fi; \
		fi; \
	done; \
	echo ""; \
	echo "FsmOS Examples Results: $$success passed, $$failed failed out of $$total total"; \
	if [ $$failed -gt 0 ]; then \
		echo ""; \
		echo "Failed examples:"; \
		for example in examples/*/; do \
			if [ -f "$$example"*.ino ]; then \
				sketch=$$(basename "$$example"); \
				if ! arduino-cli compile --fqbn arduino:avr:uno --library lib/FsmOS "$$example" >/dev/null 2>&1; then \
					echo "  - $$sketch"; \
				fi; \
			fi; \
		done; \
		exit 1; \
	fi

# Test libs examples compilation
test-libs:
	@echo ""
	@echo "=========================================="
	@echo "Testing Library Examples Compilation"
	@echo "=========================================="
	@echo "Target: Arduino Uno (arduino:avr:uno)"
	@echo ""
	@success=0; \
	failed=0; \
	total=0; \
	for lib_dir in libs/*/; do \
		if [ -d "$$lib_dir/examples" ]; then \
			for example in "$$lib_dir/examples"/*/; do \
				if [ -f "$$example"*.ino ]; then \
					lib_name=$$(basename "$$lib_dir"); \
					sketch=$$(basename "$$example"); \
					total=$$((total+1)); \
					echo -n "  [$$total] $$lib_name/$$sketch... "; \
					lib_paths="--library lib/FsmOS"; \
					if [ "$$lib_name" = "HC05" ]; then \
						lib_paths="$$lib_paths --library libs/CircularBuffers --library libs/ArduinoQueue --library libs/SimpleTimer --library libs/ArduinoMap --library libs/SafeInterrupts --library libs/StaticSerialCommands --library libs/Utilities"; \
					fi; \
					if [ "$$lib_name" = "I2C-master" ]; then \
						lib_paths="$$lib_paths --library libs/ArduinoMap --library libs/SafeInterrupts --library libs/StaticSerialCommands --library libs/SimpleTimer --library libs/Utilities"; \
					fi; \
					if [ "$$lib_name" = "MemoryUsage" ]; then \
						lib_paths="$$lib_paths --library libs/ArduinoMap --library libs/SafeInterrupts --library libs/StaticSerialCommands --library libs/Utilities"; \
					fi; \
					if [ "$$lib_name" = "Packager" ]; then \
						lib_paths="$$lib_paths --library libs/CircularBuffers --library libs/ArduinoMap --library libs/SafeInterrupts --library libs/StaticSerialCommands --library libs/SimpleTimer --library libs/BufferedStreams --library libs/Utilities"; \
					fi; \
					if [ "$$lib_name" = "SoftSerial" ]; then \
						lib_paths="$$lib_paths --library libs/FastPin --library libs/CircularBuffers --library libs/ArduinoMap --library libs/SafeInterrupts --library libs/StaticSerialCommands --library libs/Utilities"; \
					fi; \
					if [ "$$lib_name" = "Utilities" ]; then \
						lib_paths="$$lib_paths --library libs/ArduinoMap --library libs/SafeInterrupts --library libs/StaticSerialCommands"; \
					fi; \
					if [ "$$lib_name" = "ezLED" ] || [ "$$lib_name" = "ezOutput" ]; then \
						lib_paths="$$lib_paths --library libs/ezButton"; \
					fi; \
					if [ "$$sketch" = "ESP8266_MirroredConsole" ]; then \
						echo "SKIP (ESP8266 only)"; \
						total=$$((total-1)); \
						continue; \
					fi; \
					if arduino-cli compile --fqbn arduino:avr:uno $$lib_paths --library "$$lib_dir" "$$example" >/dev/null 2>&1; then \
						echo "✓ PASS"; \
						success=$$((success+1)); \
					else \
						echo "✗ FAIL"; \
						failed=$$((failed+1)); \
					fi; \
				fi; \
			done; \
		fi; \
	done; \
	echo ""; \
	echo "Library Examples Results: $$success passed, $$failed failed out of $$total total"; \
	if [ $$failed -gt 0 ]; then \
		echo ""; \
		echo "Failed examples:"; \
		for lib_dir in libs/*/; do \
			if [ -d "$$lib_dir/examples" ]; then \
				for example in "$$lib_dir/examples"/*/; do \
					if [ -f "$$example"*.ino ]; then \
						lib_name=$$(basename "$$lib_dir"); \
						sketch=$$(basename "$$example"); \
						lib_paths="--library lib/FsmOS"; \
						if [ "$$lib_name" = "HC05" ]; then \
							lib_paths="$$lib_paths --library libs/CircularBuffers --library libs/ArduinoQueue --library libs/SimpleTimer --library libs/ArduinoMap --library libs/SafeInterrupts --library libs/StaticSerialCommands --library libs/Utilities"; \
						fi; \
						if [ "$$lib_name" = "I2C-master" ]; then \
							lib_paths="$$lib_paths --library libs/ArduinoMap --library libs/SafeInterrupts --library libs/StaticSerialCommands --library libs/SimpleTimer --library libs/Utilities"; \
						fi; \
						if [ "$$lib_name" = "MemoryUsage" ]; then \
							lib_paths="$$lib_paths --library libs/ArduinoMap --library libs/SafeInterrupts --library libs/StaticSerialCommands --library libs/Utilities"; \
						fi; \
						if [ "$$lib_name" = "Packager" ]; then \
							lib_paths="$$lib_paths --library libs/CircularBuffers --library libs/ArduinoMap --library libs/SafeInterrupts --library libs/StaticSerialCommands --library libs/SimpleTimer --library libs/BufferedStreams --library libs/Utilities"; \
						fi; \
						if [ "$$lib_name" = "SoftSerial" ]; then \
							lib_paths="$$lib_paths --library libs/FastPin --library libs/CircularBuffers --library libs/ArduinoMap --library libs/SafeInterrupts --library libs/StaticSerialCommands --library libs/Utilities"; \
						fi; \
						if [ "$$lib_name" = "Utilities" ]; then \
							lib_paths="$$lib_paths --library libs/ArduinoMap --library libs/SafeInterrupts --library libs/StaticSerialCommands"; \
						fi; \
						if [ "$$lib_name" = "ezLED" ] || [ "$$lib_name" = "ezOutput" ]; then \
							lib_paths="$$lib_paths --library libs/ezButton"; \
						fi; \
						if ! arduino-cli compile --fqbn arduino:avr:uno $$lib_paths --library "$$lib_dir" "$$example" >/dev/null 2>&1; then \
							echo "  - $$lib_name/$$sketch"; \
						fi; \
					fi; \
				done; \
			fi; \
		done; \
		exit 1; \
	fi

# Test all compilation (examples + libs)
test-all: test-compile
	@echo ""
	@echo "=========================================="
	@echo "✓ All compilation tests passed!"
	@echo "=========================================="

=======
>>>>>>> 65bdf6ed5a850446de0e3034b2d80def63f1efd6:FsmOS/Makefile
# Clean documentation
clean:
	@echo "Cleaning documentation..."
	rm -rf docs/
	@echo "Documentation cleaned"

# Help
help:
<<<<<<< HEAD:Makefile
	@echo "FsmOS Makefile"
	@echo "=============="
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Generate documentation (default)"
	@echo "  docs         - Generate documentation"
	@echo "  test-compile - Test compilation of all (FsmOS + examples + libs)"
	@echo "  test-fsmos    - Test FsmOS library compilation only"
	@echo "  test-examples - Test compilation of FsmOS examples only"
	@echo "  test-libs     - Test compilation of library examples only"
	@echo "  test-all      - Alias for test-compile"
	@echo "  clean        - Remove generated documentation"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Requirements:"
	@echo "  - Doxygen 1.9.8 or higher (for docs)"
	@echo "  - Arduino CLI (for compilation tests)"
	@echo ""
	@echo "Usage:"
	@echo "  make              # Generate documentation"
	@echo "  make test-compile # Test all examples compilation"
	@echo "  make test-examples # Test FsmOS examples only"
	@echo "  make test-libs    # Test library examples only"
	@echo "  make clean        # Clean documentation"
	@echo "  make help         # Show help"
=======
	@echo "FsmOS Documentation Makefile"
	@echo "============================="
	@echo ""
	@echo "Available targets:"
	@echo "  all     - Generate documentation (default)"
	@echo "  docs    - Generate documentation"
	@echo "  clean   - Remove generated documentation"
	@echo "  help    - Show this help message"
	@echo ""
	@echo "Requirements:"
	@echo "  - Doxygen 1.9.8 or higher"
	@echo ""
	@echo "Usage:"
	@echo "  make          # Generate documentation"
	@echo "  make clean    # Clean documentation"
	@echo "  make help     # Show help"
>>>>>>> 65bdf6ed5a850446de0e3034b2d80def63f1efd6:FsmOS/Makefile
