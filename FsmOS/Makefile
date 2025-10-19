# Makefile for FsmOS Documentation
# FsmOS v1.3.0 - Standalone Library Documentation

.PHONY: all clean docs help

# Default target
all: docs

# Generate documentation
docs:
	@echo "Generating FsmOS documentation..."
	doxygen Doxyfile
	@echo "Documentation generated in docs/html/"
	@echo "Open docs/html/index.html to view"

# Clean documentation
clean:
	@echo "Cleaning documentation..."
	rm -rf docs/
	@echo "Documentation cleaned"

# Help
help:
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
