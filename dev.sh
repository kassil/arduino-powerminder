#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
BUILD_DIR="$ROOT_DIR/build"
TEST_BUILD_DIR="$ROOT_DIR/tests/build"

usage() {
    cat <<'EOF'
Usage: ./dev.sh <command>

Commands:
  clean        Remove firmware build artifacts
  clean-test   Remove host-test build artifacts
  clean-all    Remove both firmware and host-test artifacts
  build        Build AVR firmware (Docker toolchain)
  build-test   Configure and build host-side unit tests
  test         Run host-side unit tests
  check        Build firmware + build tests + run tests
EOF
}

do_build() {
    docker run --rm -v /home/kevin/programming/atmel_avr/arduino-powerminder:/home/builder/project \
        arduino-cmake /bin/bash -lc "cd /home/builder/project && cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=avr-gcc-toolchain.cmake && make -C build -j$(nproc)"
}

do_build_test() {
    cmake -S "$ROOT_DIR/tests" -B "$TEST_BUILD_DIR"
    cmake --build "$TEST_BUILD_DIR"
}

do_test() {
    ctest --test-dir "$TEST_BUILD_DIR" --output-on-failure
}

case "${1:-}" in
    clean)
        rm -rf "$BUILD_DIR"
        ;;
    clean-test)
        rm -rf "$TEST_BUILD_DIR"
        ;;
    clean-all)
        rm -rf "$BUILD_DIR" "$TEST_BUILD_DIR"
        ;;
    build)
        do_build
        ;;
    build-test)
        do_build_test
        ;;
    test)
        do_test
        ;;
    check)
        do_build
        do_build_test
        do_test
        ;;
    *)
        usage
        exit 1
        ;;
esac
