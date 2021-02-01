#include <config.h>
#include <errno.h>
#include "lib/dpif.h"
#include "lib/netdev-vport.h"
#include "lib/odp-util.h"
#include "openvswitch/vlog.h"
#include "openvswitch/shash.h"
#include "p4proto.h"
#include "p4proto-dpif.h"
#include "p4proto-provider.h"
#include "util.h"

VLOG_DEFINE_THIS_MODULE(p4proto_dpif);

/* ## --------------------------------------- ## */
/* ## p4proto-dpif helper structures definition. ## */
/* ## --------------------------------------- ## */

static struct shash p4proto_dpif_classes =
    SHASH_INITIALIZER(&p4proto_dpif_classes);


