#!/bin/sh
# Builds the Game (Development, which the Editor's Launch Game starts: GameBuilder.cpp) and the libraries it needs, as
# BuildGame.bat does on Windows. Run from anywhere; the Makefiles come from GenerateSolutions.sh.
cd "$(dirname "$0")" || exit 1

if [ ! -f Makefile ]; then
	echo "Makefile not found. Run GenerateSolutions.sh first."
	exit 1
fi

exec make config=development -j"$(getconf _NPROCESSORS_ONLN)" --no-print-directory Game
