#!/bin/sh

gcc_version() {
    local cc
    if [ -z "$1" ]; then
        return
    fi

    local cmd="$1"
    if command -v "$cmd" 2>&1 >/dev/null; then
        ver=$("$cmd" --version 2> /dev/null | head -n 1)
        # some tools (eg. openocd) print version info to stderr
        if [ -z "$ver" ]; then
            ver=$("$cmd" --version 2>&1 | head -n 1)
        fi
        if [ -z "$ver" ]; then
            ver="error"
        fi
    else
        cc=$1-gcc
    fi
    local ver=
    if command -v "$cc" 2>&1 >/dev/null; then
        ver=$("$cc" --version | head -n 1)
    fi
    if [ -z "$ver" ]; then
        ver=missing/error
    fi
    printf "%s" "$ver"
}

get_define() {
    local cc="$1"
    local line=
    if command -v "$cc" 2>&1 >/dev/null; then
        line=$(echo "$3" | "$cc" -x c -include "$2" -E -o - - 2>&1 | sed -e '/^[   ]*#/d' -e '/^[  ]*$/d')
    fi
    if [ -z "$line" ]; then
        line=missing/error
    fi
    printf "%s" "$line"
}

get_kernel_info() {
    uname -mprs
}

get_os_info() {
    local os="$(uname -s)"
    local osname="unknown"
    local osvers="unknown"
    if [ "$os" = "Linux" ]; then
        osname="$(cat /etc/os-release | grep ^NAME= | awk -F'=' '{print $2}')"
        osvers="$(cat /etc/os-release | grep ^VERSION= | awk -F'=' '{print $2}')"
    elif [ "$os" = "Darwin" ]; then
        osname="$(sw_vers -productName)"
        osvers="$(sw_vers -productVersion)"
    fi
    printf "%s %s" "$osname" "$osvers"
}

newlib_version() {
    if [ -z "$1" ]; then
        cc=gcc
    else
        cc=$1-gcc
    fi
    printf "%s" "$(get_define "$cc" newlib.h _NEWLIB_VERSION)"
}

avr_libc_version() {
    if [ -z "$1" ]; then
        cc=gcc
    else
        cc=$1-gcc
    fi
    printf "%s (%s)" "$(get_define "$cc" avr/version.h __AVR_LIBC_VERSION_STRING__)" "$(get_define "$cc" avr/version.h __AVR_LIBC_DATE_STRING__)"
}

cppcheck_version() {
    local cmd="cppcheck"
    if command -v "$cmd" 2>&1 >/dev/null; then
        ver=$("$cmd" --version | head -n 1)
    else
        ver="missing"
    fi

    printf "%s" "$ver"
}

<<<<<<< HEAD
printf "\n"
# print operating system information
printf "%s\n" "Operating System Environment"
printf "%s\n" "-----------------------------"
printf "%23s: %s\n" "Operating System" "$(get_os_info)"
printf "%23s: %s\n" "Kernel" "$(get_kernel_info)"
printf "\n"

printf "%s\n" "Installed compiler toolchains"
printf "%s\n" "-----------------------------"
printf "%23s: %s\n" "native gcc" "$(get_cmd_version gcc)"
for p in arm-none-eabi avr mips-mti-elf msp430 riscv-none-embed; do
    printf "%23s: %s\n" "$p-gcc" "$(get_cmd_version ${p}-gcc)"
=======
spatch_version() {
    local cmd="spatch"
    if command -v "$cmd" 2>&1 >/dev/null; then
        ver=$("$cmd" --version | head -n 1)
    else
        ver="missing"
    fi

    printf "%s" "$ver"
}

printf "%s\n" "Installed toolchain versions"
printf "%s\n" "----------------------------"
VER=$(gcc --version | head -n 1)
if [ -n "$VER" ]; then
    printf "%20s: %s\n" "native gcc" "$VER"
fi
for p in msp430 avr arm-none-eabi mips-mti-elf; do
    printf "%20s: %s\n" "$p-gcc" "$(gcc_version "$p")"
>>>>>>> Fix dependencies, make javascript example run
done
VER=$(clang --version | head -n 1)
if [ -n "$VER" ]; then
    printf "%20s: %s\n" "clang" "$VER"
fi

for p in arm-none-eabi mips-mti-elf; do
    printf "%20s: %s\n" "$p-newlib" "$(newlib_version "$p")"
done
<<<<<<< HEAD
# avr libc version
printf "%23s: %s\n" "avr-libc" "$(avr_libc_version avr-gcc)"
# tools
printf "\n"
printf "%s\n" "Installed development tools"
printf "%s\n" "---------------------------"
for c in cmake cppcheck doxygen flake8 git openocd python python2 python3; do
    printf "%23s: %s\n" "$c" "$(get_cmd_version $c)"
=======
for p in avr; do
    printf "%20s: %s\n" "$p-libc" "$(avr_libc_version "$p")"
>>>>>>> Fix dependencies, make javascript example run
done
printf "%20s: %s\n" "cppcheck" "$(cppcheck_version)"
printf "%20s: %s\n" "coccinelle" "$(spatch_version)"
exit 0
