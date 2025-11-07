#!/bin/bash
# Script to integrate an external library into FsmOS framework
# Usage: ./integrate_library.sh <github_url> [library_name]

set -e

GITHUB_URL="$1"
LIBRARY_NAME="$2"

if [ -z "$GITHUB_URL" ]; then
    echo "Usage: $0 <github_url> [library_name]"
    echo "Example: $0 https://github.com/user/library.git MyLibrary"
    exit 1
fi

# Extract library name from URL if not provided
if [ -z "$LIBRARY_NAME" ]; then
    LIBRARY_NAME=$(basename "$GITHUB_URL" .git)
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
LIBS_DIR="$SCRIPT_DIR/../libs"
LIB_DIR="$LIBS_DIR/$LIBRARY_NAME"

echo "Integrating library: $LIBRARY_NAME"
echo "From: $GITHUB_URL"
echo "To: $LIB_DIR"

# Create libs directory if it doesn't exist
mkdir -p "$LIBS_DIR"

# Clone the repository
if [ -d "$LIB_DIR" ]; then
    echo "Directory $LIB_DIR already exists. Updating..."
    cd "$LIB_DIR"
    git pull
else
    echo "Cloning repository..."
    git clone "$GITHUB_URL" "$LIB_DIR"
fi

echo "Library integrated successfully!"
echo "Location: $LIB_DIR"
echo ""
echo "Next steps:"
echo "1. Review the library structure"
echo "2. Check for compatibility with FsmOS"
echo "3. Update library.properties if needed"
echo "4. Test integration"

