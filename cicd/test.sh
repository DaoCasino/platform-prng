#! /bin/bash

set -e -o pipefail

. "${BASH_SOURCE[0]%/*}/utils.sh"

(
    ./build/test/prng_test --report_level=no --log_level=nothing -- $@
)

