#!/bin/sh

cd "$(dirname "$0")" || exit 1

if [ ! -f "../SaturnBuildTool" ]; then
    echo "BUILDTOOLS NOT FOUND!"
    exit 1
fi

../SaturnBuildTool "$@"
ERRNO=$?

# An error code of 2 is not a failure, it means that the BuildTool has nothing todo. 
# However MSBuild will treat an error code of two as an erorr, so we lie to it.
if [ "$ERRNO" -eq 2 ]; then
    exit 0
fi

exit "$ERRNO"
