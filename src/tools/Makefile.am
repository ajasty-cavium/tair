AM_CPPFLAGS= -I${top_srcdir}/src \
             -I$(easy_dir)/include/easy \
      -I$(TBLIB_ROOT)/include/tbsys \
      -I${top_srcdir}/src/common \
      -I${top_srcdir}/src/packets \
      -I${top_srcdir}/src/client \
      -I${top_srcdir}/src/dataserver \
      -I${top_srcdir}/src/statserver/include \
      -I${top_srcdir}/src/plugin
AM_CXXFLAGS= -Werror -fPIC

if WITH_COMPRESS
COMPRESS_LDFLAGS= -lsnappy
endif

LDADD=$(top_srcdir)/src/client/.libs/libtairclientapi.a \
    $(top_srcdir)/src/common/libtair_common.a \
    $(top_builddir)/src/common/libsnappy.a \
    $(TBLIB_ROOT)/lib/libtbsys.a \
    $(easy_dir)/lib64/libeasy.a \
    $(COMPRESS_LDFLAGS) \
    -lz -lrt

AM_LDFLAGS=-lpthread -lcurl -lssl -lz -lrt ${GCOV_LIB}

sbin_PROGRAMS=dump_transfer lens mtair parse_admin_conf

dump_transfer_SOURCES=dump_transfer.cpp
dump_transfer_LDADD=${LDADD}

lens_SOURCES=lens.cpp
lens_LDADD=${LDADD}
mtair_SOURCES=mtair.cpp

parse_admin_conf_SOURCES=parse_admin_conf.cpp
parse_admin_conf_LDADD=${LDADD}
