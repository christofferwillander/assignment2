#!/bin/sh

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../lib
./main "CREATE TABLE students (id INT, first_name VARCHAR(7), last_name VARCHAR(8));"

