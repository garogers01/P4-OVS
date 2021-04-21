#include <config.h>
#include <errno.h>
#include <string.h>

#include "p4proto.h"
#include "p4proto-provider.h"
#include "openvswitch/vlog.h"

VLOG_DEFINE_THIS_MODULE(p4proto);

void
p4proto_init(void)
{
    VLOG_DBG("Func called: %s", __func__);
    // TODO: P4runtime(grpc) Server interaction initialize here.
}

void
p4proto_deinit(void)
{
    VLOG_DBG("Func called: %s", __func__);
    //TODO: GrpcServerShutdown();
    //TODO: GrpcServerCleanup();
}

// TODO: Use right protoype after stratum integration
// p4proto_run(struct p4proto *p4proto)
int
p4proto_run(void)
{
    VLOG_DBG("Func called: %s", __func__);
}

// TODO: Use right protoype after stratum integration
// p4proto_create(const char *datapath_name, const char *datapath_type,
//         uint64_t *requested_dev_id, struct p4proto **p4protop)
//    OVS_EXCLUDED(p4proto_mutex)

/* FIXME: Fix prototype, arg - type, name, dev_id, p4proto pointer
* P4proto struct need to be filled here
* Get dev_id via ovs-p4ctl OR use "dummy" and update later during grpc call.	
*/
int
p4proto_create(void)
{
    VLOG_DBG("Func called: %s", __func__);
}

// TODO: Use right protoype after stratum integration
// p4proto_destroy(struct p4proto *p, bool del)
void
p4proto_destroy(void)
{
    VLOG_DBG("Func called: %s", __func__);
}

// TODO: Use right protoype after stratum integration
// p4proto_delete(const char *name, const char *type)
int
p4proto_delete(void)
{
    VLOG_DBG("Func called: %s", __func__);
}

void 
p4proto_exit(void)
{
    VLOG_DBG("Func called: %s", __func__);
}
