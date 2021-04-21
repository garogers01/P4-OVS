#ifndef P4PROTO_H
#define P4PROTO_H 1

#include "openvswitch/thread.h"
#include "openvswitch/types.h"
#include "netdev.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* Needed for the lock annotations. */
extern struct ovs_mutex p4proto_mutex;

struct p4proto;

/* FIXME: Use right prototypes after stratum integration */
void p4proto_init(void);

void p4proto_deinit(void);

int p4proto_run(void);

int p4proto_create(void);

void p4proto_destroy(void);

int p4proto_delete(void);

void p4proto_exit(void);

#ifdef  __cplusplus
}
#endif

#endif /* p4proto.h */
