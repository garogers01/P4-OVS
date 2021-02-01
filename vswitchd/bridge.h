/* Copyright (c) 2008, 2009, 2010, 2011, 2012, 2014 Nicira, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VSWITCHD_BRIDGE_H
#define VSWITCHD_BRIDGE_H 1

#include <stdbool.h>

#include <config.h>
#include "bridge.h"
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>

#include "async-append.h"
#include "bfd.h"
#include "bitmap.h"
#include "cfm.h"
#include "connectivity.h"
#include "coverage.h"
#include "daemon.h"
#include "dirs.h"
#include "dpif.h"
#include "dpdk.h"
#include "hash.h"
#include "openvswitch/hmap.h"
#include "hmapx.h"
#include "if-notifier.h"
#include "jsonrpc.h"
#include "lacp.h"
#include "mac-learning.h"
#include "mcast-snooping.h"
#include "netdev.h"
#include "netdev-offload.h"
#include "nx-match.h"
#include "ofproto/bond.h"
#include "ofproto/ofproto.h"
#include "openvswitch/dynamic-string.h"
#include "openvswitch/list.h"
#include "openvswitch/meta-flow.h"
#include "openvswitch/ofp-print.h"
#include "openvswitch/ofpbuf.h"
#include "openvswitch/vconn.h"
#include "openvswitch/vlog.h"
#include "lib/vswitch-idl.h"
#include "ovs-lldp.h"
#include "ovs-numa.h"
#include "packets.h"
#include "openvswitch/poll-loop.h"
#include "seq.h"
#include "sflow_api.h"
#include "sha1.h"
#include "openvswitch/shash.h"
#include "smap.h"
#include "socket-util.h"
#include "stream.h"
#include "stream-ssl.h"
#include "sset.h"
#include "system-stats.h"

struct simap;

struct bridge {
    struct hmap_node node;      /* In 'all_bridges'. */
    char *name;                 /* User-specified arbitrary name. */
    char *type;                 /* Datapath type. */
    struct eth_addr ea;         /* Bridge Ethernet Address. */
    struct eth_addr default_ea; /* Default MAC. */
    const struct ovsrec_bridge *cfg;

    /* OpenFlow switch processing. */
    struct ofproto *ofproto;    /* OpenFlow switch. */

    /* Bridge ports. */
    struct hmap ports;          /* "struct port"s indexed by name. */
    struct hmap ifaces;         /* "struct iface"s indexed by ofp_port. */
    struct hmap iface_by_name;  /* "struct iface"s indexed by name. */

    /* Port mirroring. */
    struct hmap mirrors;        /* "struct mirror" indexed by UUID. */

    /* Auto Attach */
    struct hmap mappings;       /* "struct" indexed by UUID */

    /* Used during reconfiguration. */
    struct shash wanted_ports;

    /* Synthetic local port if necessary. */
    struct ovsrec_port synth_local_port;
    struct ovsrec_interface synth_local_iface;
    struct ovsrec_interface *synth_local_ifacep;
};

void bridge_init(const char *remote);
void bridge_exit(bool delete_datapath);

void bridge_run(void);
void bridge_wait(void);

void bridge_get_memory_usage(struct simap *usage);

#endif /* bridge.h */
