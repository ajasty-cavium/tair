AM_CPPFLAGS= -I$(easy_dir)/include/easy \
						 -I$(TBLIB_ROOT)/include/tbsys \
						 -Wall

AM_LDFLAGS= -rdynamic ${GCOV_LIB} ${TCMALLOC_LDFLAGS}

CXX_PROTOS = echo_message.pb.h echo_message.pb.cc
BUILT_SOURCES = $(CXX_PROTOS)
CLEANFILES = $(CXX_PROTOS)

%.pb.cc %.pb.h: %.proto
		$(PROTOC) --proto_path=$(dir $^) --cpp_out=$(dir $^) $^



noinst_LIBRARIES=libtairrpc.a

libtairrpc_a_SOURCES=mongoose.c mongoose.h echo_command.hpp    reference_count.cpp  rpc_client_context.cpp  rpc_command_factory.cpp rpc_command_factory.h  \
			rpc_dispatcher.cpp  rpc_future.cpp  rpc_packet.cpp  rpc_server_context.cpp  rpc_session.h  unordered_map.h \
			echo_message.pb.h echo_message.pb.cc reference_count.h    rpc_client_context.h    rpc_command.hpp  rpc_thread.hpp \
			rpc_http_console.h rpc_http_console.cpp http_demo_command.h \
			rpc_context.h rpc_context.cpp \
			rpc_dispatcher_eio.h rpc_dispatcher_eio.cpp rpc_dispatcher.h    rpc_future.h    rpc_packet.h    rpc_server_context.h    tair_rpc.h rpc_command_factory.cpp tair_message.h

sbin_PROGRAMS=server_demo

server_demo_LDADD=libtairrpc.a \
			$(TBLIB_ROOT)/lib/libtbsys.a \
			$(easy_dir)/lib64/libeasy.a \
			-L/usr/lib64 -lprotobuf -lssl -lpthread -lrt -lz
server_demo_SOURCES=server_demo.cpp
