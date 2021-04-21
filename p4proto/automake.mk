lib_LTLIBRARIES += p4proto/libp4proto.la

p4proto_libp4proto_la_LDFLAGS = \
        $(OVS_LTINFO) \
        -Wl,--version-script=$(top_builddir)/p4proto/libp4proto.sym \
        $(AM_LDFLAGS)

p4proto_libp4proto_la_SOURCES = \
    p4proto/p4proto-provider.h \
    p4proto/p4proto.c \
    p4proto/p4proto.h

p4proto_libp4proto_la_CPPFLAGS = $(AM_CPPFLAGS)
p4proto_libp4proto_la_CFLAGS = $(AM_CFLAGS)

pkgconfig_DATA += \
	p4proto/libp4proto.pc

CLEANFILES += p4proto/libp4proto.sym
CLEANFILES += p4proto/libp4proto.pc
