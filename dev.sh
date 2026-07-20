#!/bin/sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
BUILD_DIR="$ROOT_DIR/build"
TEST_BUILD_DIR="$ROOT_DIR/tests/build"
DOCKER_IMAGE="${DOCKER_IMAGE:-arduino-cmake:latest}"
DOCKERFILE_PATH="$ROOT_DIR/Dockerfile"

usage() {
    cat <<'EOF'
Usage: ./dev.sh <command>

Commands:
  clean        Remove firmware build artifacts
  clean-test   Remove host-test build artifacts
  clean-all    Remove both firmware and host-test artifacts
  image-build  Build Docker toolchain image used for firmware build
  build        Build AVR firmware (Docker toolchain)
  build-relaxed Build AVR firmware with project warnings-as-errors disabled
  build-test   Configure and build host-side unit tests
  test         Run host-side unit tests
  check        Build firmware + build tests + run tests
  check-relaxed Build firmware (relaxed) + build tests + run tests
EOF
}

ensure_docker_image() {
    if docker image inspect "$DOCKER_IMAGE" >/dev/null 2>&1; then
        return 0
    fi

    echo "Docker image '$DOCKER_IMAGE' not found locally. Run './dev.sh image-build' first." >&2
    exit 1
}

do_image_build() {
    docker build -t "$DOCKER_IMAGE" -f "$DOCKERFILE_PATH" "$ROOT_DIR"
}

do_build_with_args() {
    extra_cmake_args="$1"
    ensure_docker_image
    docker run --rm -v "$ROOT_DIR":/home/builder/project \
                --user "$(id -u):$(id -g)" \
        "$DOCKER_IMAGE" /bin/bash -lc "cd /home/builder/project && cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=avr-gcc-toolchain.cmake ${extra_cmake_args} && make -C build -j\$(nproc)"
}

do_build() {
    do_build_with_args ""
}

do_build_relaxed() {
    do_build_with_args "-DPROJECT_WARNINGS_AS_ERRORS=OFF"
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
    image-build)
        do_image_build
        ;;
    build)
        do_build
        ;;
    build-relaxed)
        do_build_relaxed
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
    check-relaxed)
        do_build_relaxed
        do_build_test
        do_test
        ;;
    *)
        usage
        exit 1
        ;;
esac
