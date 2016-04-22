AM_CPPFLAGS= -I/$(easy_dir)/include/easy \
			 -I$(TBLIB_ROOT)/include/tbsys \
			 -I${top_srcdir}/src/client \
			 -I${top_srcdir}/src \
			 -I${top_srcdir}/src/rpc \
			 -I${top_srcdir}/src/statserver/include \
			 -I${top_srcdir}/src/common \
			 -Wall \
			 -I/usr/include/mysql	\
			 -I${top_srcdir}/src/packets
if WITH_TCMALLOC
AM_CPPFLAGS += -DWITH_TCMALLOC
endif
AM_CXXFLAGS=-Werror
AM_LDFLAGS= -rdynamic ${GCOV_LIB} ${TCMALLOC_LDFLAGS}

LDADD=$(top_builddir)/src/client/.libs/libtairclientapi.a \
			$(top_builddir)/src/rpc/libtairrpc.a \
			$(top_builddir)/src/common/libsnappy.a \
			$(TBLIB_ROOT)/lib/libtbsys.a \
			$(easy_dir)/lib64/libeasy.a \
			-L/usr/lib64/mysql -lmysqlclient	\
			-L/usr/lib64 -lprotobuf -lz -lrt -lssl -lpthread \
			-ldl

sbin_PROGRAMS=tair_administrator
tair_administrator_SOURCES=main.cpp administrator_client.cpp administrator_client.h \
													 murmurhash2.h murmurhash2.cpp \
													 time_event_scheduler.h time_event_scheduler.cpp \
													 administrator_initializer.h administrator_initializer.cpp \
													 http_config_helper.h http_config_helper.cpp  \
													 http_set_config.h http_set_config.cpp        \
													 http_del_config.h http_del_config.cpp        \
													 http_get_config.h http_get_config.cpp        \
													 http_check_config.h http_check_config.cpp    \
													 http_check_task.h http_check_task.cpp        \
													 http_gettair_config.h http_gettair_config.cpp   \
													 tair_manager.h tair_manager.cpp				\
													 task_scheduler.h task_scheduler.cpp				\
													 status.h																			\
													 sync_worker.h sync_worker.cpp \
													 grab_group_command.h grab_group_command.cpp \
													 notify_configuration_command.h notify_configuration_command.cpp \
													 administrator_handler.h administrator_handler.cpp \
							 http_cluster_manager.cpp http_cluster_manager.h
