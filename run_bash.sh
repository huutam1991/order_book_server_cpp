#!/bin/bash

chmod 777 generate_server_certificate.sh
./generate_server_certificate.sh

# Detect port
if [[ "$PROD" == "true" ]]; then
    PORT=443
else
    PORT=8080
fi

# perf stat -e cycles,instructions,task-clock,context-switches ./order_book_server_cpp "$PORT" web_data
./order_book_server_cpp "$PORT" web_data
