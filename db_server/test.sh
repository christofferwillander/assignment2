#!/bin/sh

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:lib
./demo "CREATE TABLE students (id INT, first_name VARCHAR(7), last_name VARCHAR(8));"
./demo ".tables"
./demo ".schema students"
./demo "DROP TABLE students;"
./demo "INSERT INTO students VALUES (42, 'Oscar', 'Svensson');"
./demo "SELECT * FROM students;"
./demo "CREATE TABLE students (id INT, first_name VARCHAR(7), last_name VARCHAR(8), PRIMARY KEY(id));"
./demo "SELECT first_name, last_name FROM students;"
./demo "DELETE FROM students WHERE id=42;"
./demo "UPDATE students SET first_name='Emil', last_name='Johansson' WHERE id=42;"
