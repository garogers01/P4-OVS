lib_LTLIBRARIES += p4proto/p4rt/libp4rt.la

p4proto_p4rt_libp4rt_la_LDFLAGS = \
        $(OVS_LTINFO) \
        -Wl,--version-script=$(top_builddir)/p4proto/p4rt/libp4rt.sym \
        $(AM_LDFLAGS)

LIB_ABSL := -labsl_strings -labsl_synchronization -labsl_graphcycles_internal \
            -labsl_stacktrace -labsl_symbolize -labsl_malloc_internal \
            -labsl_debugging_internal -labsl_demangle_internal -labsl_time \
            -labsl_strings_internal -labsl_throw_delegate \
            -labsl_base -labsl_spinlock_wait -labsl_int128 -labsl_raw_logging_internal \
            -labsl_log_severity -labsl_civil_time -labsl_civil_time -labsl_time_zone \
            -labsl_status -labsl_cord -labsl_cord_internal -labsl_cordz_info \
            -labsl_cordz_handle -labsl_cordz_sample_token -labsl_cordz_functions \
            -labsl_exponential_biased

p4proto_p4rt_libp4rt_la_LIBADD = $(LIB_ABSL)
p4proto_p4rt_libp4rt_la_LIBADD += -lgrpc -lprotobuf -lglog -lgflags -lgrpc++

SUFFIXES += .proto
EXTRA_DIST += p4proto/p4rt/utils/error.proto
EXTRA_DIST += p4proto/p4rt/common/forwarding_pipeline_configs.proto
EXTRA_DIST += p4proto/p4rt/README
EXTRA_DIST += p4proto/p4rt/LICENSE

p4proto_p4rt_libp4rt_la_SOURCES = \
    p4proto/p4rt/utils/cleanup.h \
    p4proto/p4rt/utils/error.cc \
    p4proto/p4rt/utils/error.h \
    p4proto/p4rt/utils/error.pb.cc \
    p4proto/p4rt/utils/error.pb.h \
    p4proto/p4rt/utils/integral_types.h \
    p4proto/p4rt/utils/logging.cc \
    p4proto/p4rt/utils/logging.h \
    p4proto/p4rt/utils/macros.h \
    p4proto/p4rt/utils/posix_error_space.cc \
    p4proto/p4rt/utils/posix_error_space.h \
    p4proto/p4rt/utils/status.cc \
    p4proto/p4rt/utils/status.h \
    p4proto/p4rt/utils/status_macros.cc \
    p4proto/p4rt/utils/status_macros.h \
    p4proto/p4rt/utils/statusor.cc \
    p4proto/p4rt/utils/statusor.h \
    p4proto/p4rt/utils/utils.cc \
    p4proto/p4rt/utils/utils.h \
    p4proto/p4rt/proto/google/rpc/code.pb.cc \
    p4proto/p4rt/proto/google/rpc/code.pb.h \
    p4proto/p4rt/proto/google/rpc/status.pb.cc \
    p4proto/p4rt/proto/google/rpc/status.pb.h \
    p4proto/p4rt/proto/p4/config/v1/p4info.pb.cc \
    p4proto/p4rt/proto/p4/config/v1/p4info.pb.h \
    p4proto/p4rt/proto/p4/config/v1/p4types.pb.cc \
    p4proto/p4rt/proto/p4/config/v1/p4types.pb.h \
    p4proto/p4rt/proto/p4/v1/p4data.pb.cc \
    p4proto/p4rt/proto/p4/v1/p4data.pb.h \
    p4proto/p4rt/proto/p4/v1/p4runtime.grpc.pb.cc \
    p4proto/p4rt/proto/p4/v1/p4runtime.grpc.pb.h \
    p4proto/p4rt/proto/p4/v1/p4runtime.pb.cc \
    p4proto/p4rt/proto/p4/v1/p4runtime.pb.h \
    p4proto/p4rt/common/channel.h \
    p4proto/p4rt/common/channel_internal.h \
    p4proto/p4rt/common/channel_writer_wrapper.h \
    p4proto/p4rt/common/forwarding_pipeline_configs.pb.cc \
    p4proto/p4rt/common/forwarding_pipeline_configs.pb.h \
    p4proto/p4rt/common/server_writer_wrapper.h \
    p4proto/p4rt/common/writer_interface.h \
    p4proto/p4rt/p4_service.cc \
    p4proto/p4rt/p4_service.h \
    p4proto/p4rt/p4_service_interface.cc \
    p4proto/p4rt/p4_service_interface.h \
    p4proto/p4rt/p4_service_utils.h

BUILT_SOURCES += \
	p4proto/p4rt/utils/error.pb.cc \
	p4proto/p4rt/utils/error.pb.h \
	p4proto/p4rt/proto/google/rpc/code.pb.cc \
	p4proto/p4rt/proto/google/rpc/code.pb.h \
	p4proto/p4rt/proto/google/rpc/status.pb.cc \
	p4proto/p4rt/proto/google/rpc/status.pb.h \
	p4proto/p4rt/common/forwarding_pipeline_configs.pb.cc \
	p4proto/p4rt/common/forwarding_pipeline_configs.pb.h \
	p4proto/p4rt/proto/p4/v1/p4runtime.pb.cc \
	p4proto/p4rt/proto/p4/v1/p4runtime.pb.h \
	p4proto/p4rt/proto/p4/v1/p4runtime.grpc.pb.cc \
	p4proto/p4rt/proto/p4/v1/p4runtime.grpc.pb.h \
	p4proto/p4rt/proto/p4/v1/p4data.pb.cc \
	p4proto/p4rt/proto/p4/v1/p4data.pb.h \
	p4proto/p4rt/proto/p4/config/v1/p4types.pb.cc \
	p4proto/p4rt/proto/p4/config/v1/p4types.pb.h \
	p4proto/p4rt/proto/p4/config/v1/p4info.pb.cc \
	p4proto/p4rt/proto/p4/config/v1/p4info.pb.h

p4proto/p4rt/utils/error.pb.cc p4proto/p4rt/utils/error.pb.h: $(top_srcdir)/p4proto/p4rt/utils/error.proto
	$(AM_V_GEN)@PROTOC@ -I$(top_srcdir)/p4proto/p4rt/utils/ --cpp_out=$(top_srcdir)/p4proto/p4rt/utils/ $(top_srcdir)/p4proto/p4rt/utils/error.proto

p4proto/p4rt/proto/google/rpc/code.pb.cc p4proto/p4rt/proto/google/rpc/code.pb.h: /usr/local/lib/python3.6/dist-packages/google/rpc/code.proto
	$(AM_V_GEN)@PROTOC@ -I/usr/local/lib/python3.6/dist-packages/ --cpp_out=$(top_srcdir)/p4proto/p4rt/proto/ /usr/local/lib/python3.6/dist-packages/google/rpc/code.proto

p4proto/p4rt/proto/google/rpc/status.pb.cc p4proto/p4rt/proto/google/rpc/status.pb.h: /usr/local/lib/python3.6/dist-packages/google/rpc/status.proto
	$(AM_V_GEN)@PROTOC@ -I/usr/local/lib/python3.6/dist-packages/ -I/usr/local/include --cpp_out=$(top_srcdir)/p4proto/p4rt/proto/ /usr/local/lib/python3.6/dist-packages/google/rpc/status.proto

p4proto/p4rt/common/forwarding_pipeline_configs.pb.cc p4proto/p4rt/common/forwarding_pipeline_configs.pb.h: $(top_srcdir)/p4proto/p4rt/common/forwarding_pipeline_configs.proto
	$(AM_V_GEN)@PROTOC@ -I/usr/local/lib/python3.6/dist-packages/ -I/usr/local/include -I$(top_srcdir)/p4proto/p4rt/common/ -I$(top_srcdir)/p4runtime/proto --cpp_out=$(top_srcdir)/p4proto/p4rt/common/ $(top_srcdir)/p4proto/p4rt/common/forwarding_pipeline_configs.proto

p4proto/p4rt/proto/p4/v1/p4runtime.pb.cc p4proto/p4rt/proto/p4/v1/p4runtime.pb.h: $(top_srcdir)/p4runtime/proto/p4/v1/p4runtime.proto
	$(AM_V_GEN)@PROTOC@ -I/usr/local/lib/python3.6/dist-packages/ -I/usr/local/include -I$(top_srcdir)/p4runtime/proto --cpp_out=$(top_srcdir)/p4proto/p4rt/proto/ $(top_srcdir)/p4runtime/proto/p4/v1/p4runtime.proto

p4proto/p4rt/proto/p4/v1/p4runtime.grpc.pb.cc p4proto/p4rt/proto/p4/v1/p4runtime.grpc.pb.h: $(top_srcdir)/p4runtime/proto/p4/v1/p4runtime.proto
	$(AM_V_GEN)@PROTOC@ -I/usr/local/lib/python3.6/dist-packages/ -I/usr/local/include -I$(top_srcdir)/p4runtime/proto --grpc_out=$(top_srcdir)/p4proto/p4rt/proto/ --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` $(top_srcdir)/p4runtime/proto/p4/v1/p4runtime.proto

p4proto/p4rt/proto/p4/v1/p4data.pb.cc p4proto/p4rt/proto/p4/v1/p4data.pb.h: $(top_srcdir)/p4runtime/proto/p4/v1/p4data.proto
	$(AM_V_GEN)@PROTOC@ -I$(top_srcdir)/p4runtime/proto --cpp_out=$(top_srcdir)/p4proto/p4rt/proto/ $(top_srcdir)/p4runtime/proto/p4/v1/p4data.proto

p4proto/p4rt/proto/p4/config/v1/p4types.pb.cc p4proto/p4rt/proto/p4/config/v1/p4types.pb.h: $(top_srcdir)/p4runtime/proto/p4/config/v1/p4types.proto
	$(AM_V_GEN)@PROTOC@ -I$(top_srcdir)/p4runtime/proto --cpp_out=$(top_srcdir)/p4proto/p4rt/proto/ $(top_srcdir)/p4runtime/proto/p4/config/v1/p4types.proto

p4proto/p4rt/proto/p4/config/v1/p4info.pb.cc p4proto/p4rt/proto/p4/config/v1/p4info.pb.h: $(top_srcdir)/p4runtime/proto/p4/config/v1/p4info.proto
	$(AM_V_GEN)@PROTOC@ -I$(top_srcdir)/p4runtime/proto --cpp_out=$(top_srcdir)/p4proto/p4rt/proto/ $(top_srcdir)/p4runtime/proto/p4/config/v1/p4info.proto

p4proto_p4rt_libp4rt_la_CPPFLAGS = $(AM_CPPFLAGS)
p4proto_p4rt_libp4rt_la_CPPFLAGS += -I .
p4proto_p4rt_libp4rt_la_CPPFLAGS += -I ./p4proto
p4proto_p4rt_libp4rt_la_CPPFLAGS += -I ./p4proto/p4rt
p4proto_p4rt_libp4rt_la_CPPFLAGS += -I ./p4proto/p4rt/proto
p4proto_p4rt_libp4rt_la_CPPFLAGS += -I ./p4proto/p4rt/common
p4proto_p4rt_libp4rt_la_CPPFLAGS += -I ./p4proto/p4rt/utils
p4proto_p4rt_libp4rt_la_CPPFLAGS += -I /usr/local/lib/

p4proto_p4rt_libp4rt_la_CFLAGS = $(AM_CFLAGS)

pkgconfig_DATA += \
        p4proto/p4rt/libp4rt.pc

CLEANFILES += p4proto/p4rt/libp4rt.sym
CLEANFILES += p4proto/p4rt/libp4rt.pc
