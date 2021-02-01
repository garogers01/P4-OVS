#ifndef P4PROTO_H
#define P4PROTO_H 1

#include "openvswitch/thread.h"
#include "openvswitch/types.h"
#include "netdev.h"
// #include "p4proto-switch.h" - Do we need it?

#ifdef  __cplusplus
extern "C" {
#endif

/* Needed for the lock annotations. */
extern struct ovs_mutex p4proto_mutex;

struct p4proto;

void p4proto_init(void);
void p4proto_deinit(void);
int p4proto_run();
int p4proto_create(const char *datapath, const char *type,
            uint64_t *requested_dev_id, struct p4proto **p4protop);
int p4proto_initialize_datapath(struct p4proto *p, const char *config_path,
        const char *p4info_path);
void p4proto_destroy(struct p4proto *p, bool del);
int p4proto_delete(const char *name, const char *type);
int p4proto_type_run();

#ifdef  __cplusplus
}
#endif

#endif /* p4proto.h */

