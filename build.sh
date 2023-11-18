#!/bin/sh

cc -std=c11 -g3 -Wall -I. -o raymarch main.c -lglfw -lGL -lm
