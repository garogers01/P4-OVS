#ifndef P4PROTO_PROVIDER_H
#define P4PROTO_PROVIDER_H 1

#include "openvswitch/hmap.h"
#include "lib/p4proto-objects.h"

/* Maximum number of P4 devices?? (Eg, PI = 256) */
// #define MAX_PROGS 256

struct p4proto {
    struct hmap_node hmap_node; /* In global 'all_p4protos' hmap. */
    const struct p4proto_class *p4proto_class;

    char *type;                 /* Datapath type. */
    char *name;                 /* Datapath name. */

    // TODO: Placeholder - P4Info describing a P4 program.

    /* Device ID used by P4Runtime to identify bridge. */
    uint64_t dev_id;

    /* Datapath. */
};


/* P4Runtime provider interface.
 * This interface takes the best from OpenFlow provider. */
struct p4proto_class {
};

extern const struct p4proto_class p4proto_dpif_class;

#endif /* p4proto-provider.h */
