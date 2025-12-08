FROM ubuntu:24.04

# Copy source code into container
WORKDIR /order_book_server_cpp
ADD core /order_book_server_cpp/core
ADD src /order_book_server_cpp/src
ADD z_orderbook_data /order_book_server_cpp/z_orderbook_data
ADD CMakeLists.txt /order_book_server_cpp
ADD server-certificate.crt /order_book_server_cpp
ADD server-private-key.pem /order_book_server_cpp
ADD build_bash.sh /order_book_server_cpp
ADD run_bash.sh /order_book_server_cpp

# Add lib to system path
ENV LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH

# Build source code
RUN chmod 777 build_bash.sh
RUN chmod 777 run_bash.sh
RUN ./build_bash.sh

# Add web data
ADD web_data /order_book_server_cpp/web_data

# tell the port number the container should expose
EXPOSE 443 80 8081

# run the command
CMD ["./run_bash.sh"]
