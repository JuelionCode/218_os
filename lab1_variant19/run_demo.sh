#!/bin/bash
# Скрипт демонстрации лабораторной работы для преподавателя

echo "=== 1. Сборка проекта ==="
make clean
make

echo -e "\n=== 2. Запуск программы parent ==="
echo "Подаем на вход:"
echo "  1-я строка: file1.txt (файл для child1)"
echo "  2-я строка: file2.txt (файл для child2)"
echo "  Далее строки текста для фильтрации гласных..."

./parent << 'EOF'
file1.txt
file2.txt
Operating Systems Laboratory Work 1
MAI Department 806
This text will have all vowels removed
Probability distribution sends eighty percent of lines to first child
And twenty percent of lines go to second child
Quick test line with vowels AEIOUY and numbers 12345
EOF

echo -e "\n=== 3. Проверка результата в file1.txt (child1) ==="
if [ -f file1.txt ]; then
    cat file1.txt
else
    echo "Файл file1.txt пуст или не был создан."
fi

echo -e "\n=== 4. Проверка результата в file2.txt (child2) ==="
if [ -f file2.txt ]; then
    cat file2.txt
else
    echo "Файл file2.txt пуст или не был создан."
fi

echo -e "\n=== Демонстрация успешно завершена! ==="
