# Build for the Paper 3 live-build scaffold.
#
# Products:
#   bin/iscandar-layer     the user-space translation layer
#   bin/iscandar-exerciser the page exerciser (from exerciser/)
#
# The layer links the admitted C translation: k4 and the k3
# transcription files from this repo's c-translation/, plus
# the author's gear bodies (Mark, Unmark, Commit, the check
# and driver gears) from the engine repository, which the
# c-translation README names as the source of record and
# which run_k4_c_tests.sh likewise links. The shared k3 files
# in the two places are byte-identical (verified 2026-11-11);
# the gear bodies exist only in the engine repository.
#
# The k3 headers carry __declspec (the author's Windows-first
# declaration form); the prefix header defines it away, the
# repo's established Linux pattern (see run_k4_c_tests.sh).

ENGINE_REPO ?= $(HOME)/workspace/iscandar/iscandar-engine
K3 := ../c-translation/k3
K4 := ../c-translation/k4

CC      ?= cc
CFLAGS  ?= -O2 -std=gnu99 -Wall -Wextra
WORK    := .work

GEARS := $(ENGINE_REPO)/Mark.c $(ENGINE_REPO)/Unmark.c \
         $(ENGINE_REPO)/Commit.c $(ENGINE_REPO)/RowsCheck.c \
         $(ENGINE_REPO)/PlanesCheck.c $(ENGINE_REPO)/PairsRowDriver.c \
         $(ENGINE_REPO)/PlaneSmallDriver.c

ENGINE_SRCS := $(K4)/Beam.c $(K4)/BeamCoordinate.c \
               $(K4)/CPUPointer.c $(K4)/CubeEngine.c \
               $(K3)/Engine.c $(K3)/Modes.c $(K3)/Mode.c \
               $(K3)/Nano.c $(K3)/Tiny.c $(K3)/Small.c \
               $(K3)/Large.c $(K3)/Huge.c $(K3)/Cube.c \
               $(K3)/Plane.c $(K3)/Coordinate.c \
               $(K3)/Bijection.c $(K3)/MPUPointer.c \
               $(K3)/Nothing.c $(K3)/Error.c \
               $(GEARS)

all: bin/iscandar-layer bin/iscandar-exerciser

$(WORK)/declspec.h:
	@mkdir -p $(WORK)
	printf '#define __declspec(x)\n' > $@

bin/iscandar-layer: layer/iscandar_layer.c bridge/iscandar_bridge.h $(WORK)/declspec.h
	@mkdir -p bin
	$(CC) $(CFLAGS) -include $(WORK)/declspec.h \
	    -Ibridge -I$(K4) -I$(K3) -I$(ENGINE_REPO) \
	    -o $@ layer/iscandar_layer.c $(ENGINE_SRCS)

bin/iscandar-exerciser: ../exerciser/exerciser.c
	@mkdir -p bin
	$(CC) -O2 -std=c11 -Wall -Wextra -o $@ ../exerciser/exerciser.c

self-test: bin/iscandar-layer
	./bin/iscandar-layer --self-test

clean:
	rm -rf $(WORK) bin/iscandar-layer bin/iscandar-exerciser

.PHONY: all self-test clean
