#!/bin/sh
valgrind --tool=callgrind ./bin/TrainGame --cycles 100 play figure8
