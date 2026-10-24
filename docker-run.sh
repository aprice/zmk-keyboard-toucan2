#!/bin/bash
docker run --rm -it \
	-v "$PWD":/src:ro \
	-v zmk-ws:/ws \
	-v "$PWD/out":/out \
	-w /ws \
	zmkfirmware/zmk-build-arm:stable bash

