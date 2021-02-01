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
}

void
p4proto_deinit(void)
{
  VLOG_DBG("Func called: %s", __func__);
}

// TODO: Use right protoype after "dpif" implemntation
// p4proto_run(struct p4proto *p4proto)
int
p4proto_run()
{
  VLOG_DBG("Func called: %s", __func__);
}

int
p4proto_create(const char *datapath_name, const char *datapath_type,
            uint64_t *requested_dev_id, struct p4proto **p4protop)
    OVS_EXCLUDED(p4proto_mutex)
{
  VLOG_DBG("Func called: %s", __func__);
}

int
p4proto_initialize_datapath(struct p4proto *p, const char *config_path,
        const char *p4info_path)
{
  VLOG_DBG("Func called: %s", __func__);
}

void
p4proto_destroy(struct p4proto *p, bool del)
{
  VLOG_DBG("Func called: %s", __func__);
}

int
p4proto_delete(const char *name, const char *type)
{
  VLOG_DBG("Func called: %s", __func__);
}

// TODO: Use right protoype after "dpif" implemntation
//p4proto_type_run(const char *datapath_type)
int
p4proto_type_run()
{
  VLOG_DBG("Func called: %s", __func__);
}

/* ## Functions , that maybe exposed to ovs-p4ctl? ## */
/* ## ------------------------------- ## */

static struct p4proto *
p4proto_lookup(const char *name)
{
  VLOG_DBG("Func called: %s", __func__);
}
