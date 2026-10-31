#!/bin/bash
# Скрипт демонстрации количества потоков средствами ОС (ps -T / top)

echo "=== Демонстрация потоков процесса средствами ОС Linux ==="
echo "Запуск ./kmeans --demo -t 6 в фоновом режиме..."

./kmeans --demo -t 6 > /tmp/kmeans_demo.log 2>&1 &
PID=$!
sleep 0.4

echo "PID процесса: $PID"
echo -e "\nСписок всех потоков (LWP / SPID) через утилиту 'ps -T -p $PID':"
ps -T -p $PID

THREAD_COUNT=$(ps -T -p $PID | tail -n +2 | wc -l)
echo -e "\nКоличество потоков в таблице ОС: $THREAD_COUNT (1 главный поток + 6 созданных POSIX-потоков)"

# Завершаем фоновый процесс
kill -9 $PID 2>/dev/null
wait $PID 2>/dev/null
echo -e "\n[OK] Демонстрация успешно выполнена!"
