#include <config.h>
#include <ctype.h>
#include <errno.h>
#include <sys/socket.h>
#include <net/if.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "openvswitch/ofp-parse.h"
#include "openvswitch/vlog.h"
#include "openvswitch/shash.h"
#include "util.h"

#define MGMT "mgmt"

typedef void (*p4ovs_cmdl_handler)(int argc, const char *argv[]);

struct p4ovs_cmdl_command {
    const char *name;
    const char *usage;
    int min_args;
    int max_args;
    p4ovs_cmdl_handler handler;
};

enum p4ctl_command_type {
    ADD_FLOW_COMMAND = 0,
    DEL_FLOW_COMMAND,

    MAX_ALLOWED_COMMAND,
};

/* FIXME, this structure should be in p4proto-flow.h file
 * Also, add generic hash mechanishm to store multiple keys
 * and value. For now added array's to store one key and one
 * value as per simple_l3 usecase.
 */
struct p4_flow_mod {
    uint8_t p4_device_id;    /* p4 device ID for the flow*/
    uint16_t command;        /* Command to execute */
    char *table_name;        /* P4 table name */
    struct shash key_value;  /* Key and values are stored in shash */
    struct shash action;     /* Action is stored in shash */
};

VLOG_DEFINE_THIS_MODULE(p4ctl);

/* Helper functions */
static int p4ctl_usage(const char *);
static int p4ctl_version(const char *);
static int p4ctl_list_commands(const char *);

/* Handlers for all commands */
static void p4add_flow(int argc, const char *argv[]);
static void p4del_flow(int argc, const char *argv[]);

static int
p4ctl_usage(const char *name)
{
    printf("%s: P4OVS switch management utility\n"
           "usage: %s [OPTIONS] COMMAND [ARG...]\n"
           "\nFor P4OVS switches:\n"
           "  add-flow P4_DEVICE TABLE_NAME KEY=VALUE,ACTION=KEY:VALUE,KEY..\t \
                add flow for the P4 device\n"
           "  del-flow P4_DEVICE TABLE_NAME KEY=VALUE\t \
                delete flow from the P4 device\n", name, name);

    return 0;
}

static int
p4ctl_version(const char *name)
{
    /* get version details for this new command if any */
    printf("%s: Version is **** \n", name);

    return 0;
}

static const struct p4ovs_cmdl_command all_p4ctl_commands[] = {
    { "add-flow",
        "utility to add all flows for a P4 device", 3, 3, p4add_flow},
    { "del-flow",
        "utility to delete all flows for a P4 device", 3, 3, p4del_flow},

    /* End of commands */
    { NULL, NULL, 0, 0, NULL}
};

static const struct p4ovs_cmdl_command
*get_all_p4ctl_commands(void)
{
    return all_p4ctl_commands;
}

static int
p4ctl_list_commands(const char *name)
{
    const struct p4ovs_cmdl_command *p;

    printf("%s: switch management utility commands are :\n\n", name);

    for (p = get_all_p4ctl_commands(); p->name != NULL; p++) {
        printf("%s %s\n", p->name, p->usage);
    }

    printf("\nUse command %s --help for detailed usage\n", name);
    return 0;
}

static bool
validate_tokens(char **keyp, char **valuep)
{
    if (strchr(*keyp, ':') || strchr(*keyp, '=')) {
        return false;
    }

    if (strchr(*valuep, ':') || strchr(*valuep, '=')) {
        return false;
    }

    return true;
}

static bool
parse_flow_str(struct p4_flow_mod *flow, const char *p4_device,
                const char *table_name, const char *str_,
                enum p4ctl_command_type command)
{
    char *key = NULL;
    char *value = NULL;
    char *act_str = NULL;
    char *string = xstrdup(str_);
    int p4_device_number = atoi(p4_device);

    /* Initialize flow structure to Zero's */
    memset(flow, 0, sizeof(struct p4_flow_mod));

    shash_init(&flow->key_value);
    shash_init(&flow->action);

    /* Copy CLI info to flow_mod structure */
    flow->p4_device_id = p4_device_number;
    flow->command = command;
    flow->table_name = xstrdup(table_name);

    string += strspn(string, ", \t\r\n");
    if (*string == '\0') {
        printf("Reached end of the string, invalid input\n");
        return false;
    }

    /* Extract the key and the delimiter that ends the key-value pair or begins
     * the value.  Advance the input position past the key and delimiter.
     */
    if (command == ADD_FLOW_COMMAND) {
        act_str = ofp_extract_actions(string);
        if (!act_str) {
            printf("must specify an action \n");
            return false;
        }

        while (ofputil_parse_key_value(&act_str, &key, &value)) {
            shash_add_once(&flow->action, key, value);
        }
    }

    key = value = NULL;
    while (ofputil_parse_key_value(&string, &key, &value)) {
        if (key[0] == '\0' || value[0] == '\0') {
            printf("Expected input in Key:value pair\n");
            return false;
        }

        if (!validate_tokens(&key, &value)) {
            printf("Multiple Key:value pairs need to be comma seperated\n");
            return false;
        }
        shash_add_once(&flow->key_value, key, value);
    }

    return true;
}

static bool
p4ctl_flow_device__(void)
{
    /* FIXME we need to have a gRPC mechanism to communicate between
     * p4ctl and vswitchd.
     */
    return true;
}

static bool
add_flow_device(const char *argv[])
{
    struct p4_flow_mod flow;
    bool error;

    error = parse_flow_str(&flow, argv[2], argv[3], argv[4], ADD_FLOW_COMMAND);
    if (!error) {
        printf("unknown command ovs-p4ctl '%s' use --help for help\n", argv[1]);
        return false;
    }

    /* When p4ctl_flow_device__ function is complete, pass p4_flow_mod structure
     * object as an argument. p4ctl_flow_device__(&flow)
     */
    if (p4ctl_flow_device__()) {
        return true;
    }

    return false;
}

static void
p4add_flow(int argc, const char *argv[])
{
    if (argc < 5) {
        printf("unknown command ovs-p4ctl '%s' use --help for help\n", argv[1]);
        return;
    }

    if (add_flow_device(argv)) {
        printf("Adding flow %s for p4device :%s\n", argv[3], argv[2]);
    }

    return;
}
static void
p4del_flow(int argc, const char *argv[])
{
    struct p4_flow_mod p4fm;
    bool error;

    if (argc < 4) {
        printf("unknown command ovs-p4ctl '%s' use --help for help\n", argv[1]);
        return;
    }

    error = parse_flow_str(&p4fm, argv[2], argv[3], argv[4], DEL_FLOW_COMMAND);
    if (!error) {
        printf("Unknow command ovs-p4ctl '%s' use --help for help \n", argv[1]);
        return;
    }

    /* When p4ctl_flow_device__ function is complete, pass p4_flow_mod structure
     * object as an argument. p4ctl_flow_device__(&p4fm)
     */
    if (p4ctl_flow_device__()) {
        printf("Deleting flow %s for p4_device: %s\n", argv[3], argv[2]);
    }

    return;
}

int
main(int argc, char *argv[])
{
    const struct p4ovs_cmdl_command *p;
    char *m_cmd = argv[1];

    set_program_name(argv[0]);

    printf("Starting binary : %s\n", argv[0]);

    /* Sanity check */
    if (argc < 2 || argc > 5) {
        return p4ctl_usage(argv[0]);
    } else if ((strncmp("--help", m_cmd, strlen(m_cmd)) == 0) ||
                (strncmp("-h", m_cmd, strlen(m_cmd)) == 0)) {
        return p4ctl_usage(argv[0]);
    } else if ((strncmp("--version", m_cmd, strlen(m_cmd)) == 0) ||
                (strncmp("-V", m_cmd, strlen(m_cmd)) == 0)) {
        return p4ctl_version(argv[0]);
    } else if (strncmp("list-commands", m_cmd, strlen(m_cmd)) == 0) {
        return p4ctl_list_commands(argv[0]);
    }

    for (p = get_all_p4ctl_commands(); p->name != NULL; p++) {
        if (!strncmp(p->name, m_cmd, strlen(p->name)) &&
            (strlen(p->name) == strlen(m_cmd))) {
            p->handler(argc, (const char **)argv);
            return 0;
        }
    }

    printf("unknown command ovs-p4ctl '%s'; use --help for help\n", argv[1]);

    return 0;
}
