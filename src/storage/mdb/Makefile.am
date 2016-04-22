AM_CPPFLAGS= \
						 -I${top_srcdir}/src \
						 -I$(easy_dir)/include/easy \
						 -I$(TBLIB_ROOT)/include/tbsys \
						 -I$(top_srcdir)/src/storage/fdb \
						 -I$(top_srcdir)/src/storage \
						 -I$(top_srcdir)/src/common \
						 -I${top_srcdir}/src/storage/mdb \
						 -I${top_srcdir}/src/storage/mdb/jni/include \
						 -I${top_srcdir}/src/storage/mdb/jni/include/linux

LDADD= $(TBLIB_ROOT)/lib/libtbsys.a

shm_source_list=cache_hashmap.cpp \
	        mdb_manager.cpp \
	        mem_cache.cpp \
					mdb_instance.hpp \
					mdb_instance.cpp \
		mem_pool.cpp \
		mdb_stat_manager.hpp \
		mdb_stat_manager.cpp \
		${top_srcdir}/src/common/data_dumpper.cpp \
		${top_srcdir}/src/common/stat_define.hpp ${top_srcdir}/src/common/stat_define.cpp \
		${top_srcdir}/src/common/util.hpp ${top_srcdir}/src/common/util.cpp

AM_LDFLAGS=-lpthread -lz -static-libgcc ${GCOV_LIB}
AM_CXXFLAGS=-Werror -fPIC
lib_LTLIBRARIES=libmdb.la libmdb_c.la libmdb_j.la

libmdb_la_SOURCES=${shm_source_list} mdb_factory.cpp mdb_define.cpp \
					cache_hashmap.hpp	\
					mdb_define.hpp	\
					mdb_factory.hpp	\
					mdb_manager.hpp	\
					mdb_stat.hpp	\
					mem_cache.hpp   \
					../../common/stat_helper.cpp \
					mem_pool.hpp \
					lock_guard.hpp
#libmdb_a_LDFLAGS=${AM_LDFLAGS}
libmdb_c_la_SOURCES=${shm_source_list} mdb_factory.cpp mdb_define.cpp \
					cache_hashmap.hpp	\
					mdb_define.hpp	\
					mdb_factory.hpp	\
					mdb_manager.hpp	\
					mdb_stat.hpp	\
					mem_cache.hpp   \
					mem_pool.hpp \
					libmdb_c.hpp \
					../../common/stat_helper.cpp \
					jni/include/jni.h \
					jni/include/linux/jni_md.h \
					libmdb_c.cpp
libmdb_j_la_SOURCES=${libmdb_c_la_SOURCES} com_taobao_tair_jmdb_JMdbNative.cpp com_taobao_tair_jmdb_JMdbNative.h
libmdb_j_la_LDFLAGS=$(LDADD) -lrt

include_HEADERS=mdb_factory.hpp mdb_manager.hpp libmdb_c.hpp

sbin_PROGRAMS = mdb_reader
mdb_reader_SOURCES=mdb_reader.cpp
mdb_reader_LDADD=libmdb.la $(LDADD) -lpthread -lrt
mdb_reader_LDFLAGS=-static

noinst_PROGRAMS=mdbtest mdbSlabAndAreaTest mdbAreaTest lazyClearTest libmdb_test_c

mdbtest_SOURCES=mdb_test.cpp
mdbtest_LDADD=libmdb.la $(LDADD) -lpthread -lrt
mdbtest_LDFLAGS=-static

#noinst_PROGRAMS=mdbSlabAndAreaTest
mdbSlabAndAreaTest_SOURCES=mdb_slab_test.cpp

mdbSlabAndAreaTest_LDADD=libmdb.la $(LDADD) -lpthread -lrt
mdbSlabAndAreaTest_LDFLAGS=-static

#noinst_PROGRAMS=lazyClearTest
lazyClearTest_SOURCES=lazy_clear_test.cpp
lazyClearTest_LDADD=libmdb_c.la $(LDADD) -lpthread -lrt
lazyClearTest_LDFLAGS=-static
#noinst_PROGRAMS=mdbAreaTest
mdbAreaTest_SOURCES=mdb_area_test.cpp
mdbAreaTest_LDADD=libmdb.la $(LDADD) -lpthread -lrt
mdbAreaTest_LDFLAGS=-static

libmdb_test_c_SOURCES=libmdb_test_c.cpp
libmdb_test_c_LDADD=libmdb_c.la $(LDADD) -lpthread -lrt
libmdb_test_c_LDFLAGS=-static
