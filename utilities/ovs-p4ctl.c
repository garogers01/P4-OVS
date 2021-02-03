/*
 * Copyright (c) 2008-2017, 2019-2021, Nicira, Inc.
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

#include <ctype.h>
#include <errno.h>
#include <sys/socket.h>
#include <net/if.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "daemon.h"
#include "colors.h"
#include "compiler.h"
#include "dirs.h"
#include "dp-packet.h"
#include "fatal-signal.h"
#include "nx-match.h"
#include "odp-util.h"
#include "ofp-version-opt.h"
#include "ofproto/ofproto.h"
#include "openflow/nicira-ext.h"
#include "openflow/openflow.h"
#include "openvswitch/dynamic-string.h"
#include "openvswitch/meta-flow.h"
#include "openvswitch/ofp-actions.h"
#include "openvswitch/ofp-bundle.h"
#include "openvswitch/ofp-errors.h"
#include "openvswitch/ofp-group.h"
#include "openvswitch/ofp-match.h"
#include "openvswitch/ofp-meter.h"
#include "openvswitch/ofp-msgs.h"
#include "openvswitch/ofp-monitor.h"
#include "openvswitch/ofp-port.h"
#include "openvswitch/ofp-print.h"
#include "openvswitch/ofp-util.h"
#include "openvswitch/ofp-parse.h"
#include "openvswitch/ofp-queue.h"
#include "openvswitch/ofp-switch.h"
#include "openvswitch/ofp-table.h"
#include "openvswitch/ofpbuf.h"
#include "openvswitch/shash.h"
#include "openvswitch/vconn.h"
#include "openvswitch/vlog.h"
#include "packets.h"
#include "pcap-file.h"
#include "openvswitch/poll-loop.h"
#include "random.h"
#include "sort.h"
#include "stream-ssl.h"
#include "socket-util.h"
#include "timeval.h"
#include "unixctl.h"
#include "util.h"

typedef void (*p4ovs_cmdl_handler)(int argc, const char *argv[]);

struct p4ovs_cmdl_command {
    const char *name;
    const char *usage;
    int min_args;
    int max_args;
    p4ovs_cmdl_handler handler;
};

VLOG_DEFINE_THIS_MODULE(p4ctl);

//static const struct p4ovs_cmdl_command *get_all_commands(void);

static int p4ctl_usage(const char *);
static int p4ctl_version(const char *);
static int p4ctl_list_commands(const char *);

// Handlers for all functions
static void add_flow(int argc, const char *argv[]);

static int
p4ctl_usage(const char *name)
{
    printf("%s: P4OVS switch management utility\n"
           "usage: %s [OPTIONS] COMMAND [ARG...]\n"
           "\nFor P4OVS switches:\n"
           "  add-flow SWITCH FLOW        add flow described by FLOW\n",
           name, name);

    return 0;
}

static int
p4ctl_version(const char *name)
{
    printf("%s: Version is **** \n", name);

    return 0;
}

static const struct p4ovs_cmdl_command all_p4ctl_commands[] = {
    { "add-flow", "add-flow bridge_name, utility to add all flows", 1, 1, add_flow},

    // End of commands
    { NULL, NULL, 0, 0, NULL}
};

static const struct p4ovs_cmdl_command *get_all_p4ctl_commands(void)
{
    return all_p4ctl_commands;
}

static int
p4ctl_list_commands(const char *name)
{
    const struct p4ovs_cmdl_command *p;
    printf("%s: switch management utility commands are :\n", name);
    for ( p = get_all_p4ctl_commands(); p->name != NULL; p++) {
        printf("%s %s\n", p->name, p->usage);
    }
    return 0;
}

static void
add_flow(int argc, const char *argv[])
{
    if (argc < 3) {
        printf("unknown command ovs-p4ctl '%s'; use --help for help\n", argv[1]);
        return;
    }

    printf("Adding a flow for Bridge :%s\n", argv[2]);
    return;
}

int
main(int argc, char *argv[])
{
    char *m_cmd = argv[1];

    set_program_name(argv[0]);

    printf("Starting binary : %s\n", argv[0]);

    // Sanity check
    if (argc < 2 || argc > 4)
        return p4ctl_usage(argv[0]);
    else if (strncmp("--help", m_cmd, strlen(m_cmd)) == 0 || strncmp("-h", m_cmd, strlen(m_cmd)) == 0)
        return p4ctl_usage(argv[0]);
    else if (strncmp("--version", m_cmd, strlen(m_cmd)) == 0 || strncmp("-V", m_cmd, strlen(m_cmd)) == 0)
        return p4ctl_version(argv[0]);
    else if (strncmp("list-commands", m_cmd, strlen(m_cmd)) == 0)
        return p4ctl_list_commands(argv[0]);

    const struct p4ovs_cmdl_command *p;

    for ( p = get_all_p4ctl_commands(); p->name != NULL; p++) {
        if (!strncmp(p->name, m_cmd, strlen(p->name)) && (strlen(p->name) == strlen(m_cmd)) ) {

            p->handler(argc, (const char **)argv);
	    return 0;
        }
    }

    printf("unknown command ovs-p4ctl '%s'; use --help for help\n", argv[1]);

    return 0;
}
