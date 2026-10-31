#!/bin/bash
set -e

echo "=== Running 100 lines distribution test ==="
rm -f f1.txt f2.txt

{
    echo "f1.txt"
    echo "f2.txt"
    for i in $(seq 1 100); do
        echo "Line $i: The quick brown fox jumps over the lazy dog"
    done
} | ./parent > /dev/null

count1=$(wc -l < f1.txt)
count2=$(wc -l < f2.txt)

echo "Result for 100 lines:"
echo "File 1 (pipe1, expected ~80%): $count1 lines"
echo "File 2 (pipe2, expected ~20%): $count2 lines"

echo -e "\n=== First 3 lines from f1.txt ==="
head -n 3 f1.txt

echo -e "\n=== First 3 lines from f2.txt ==="
head -n 3 f2.txt
