idf_component_register(
	SRCS
		source/crypto/hash_engine.cpp
		source/crypto/signature_engine.cpp
		source/networking/tcp_socket_acceptor.cpp
		source/networking/tcp_socket_connection.cpp
		source/networking/udp_socket.cpp
		source/tasks/receiver_node_task.cpp
		source/tasks/sender_node_task.cpp
		source/tasks/test_task.cpp
		source/tasks/wifi_ap_task.cpp
		source/wifi/wifi_access_point_handler.cpp
		source/wifi/wifi_client_handler.cpp
		source/wifi/wifi_generic_handler.cpp
		main.cpp

	INCLUDE_DIRS
		"include"
		"source"
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wpedantic)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=c++20 -Os)
