/* Copyright (c) 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017 Nicira, Inc.
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

#include <config.h>
#include "bridge.h"
#include <errno.h>
#include <inttypes.h>
#include <stdlib.h>

#include "p4proto/p4proto.h"

VLOG_DEFINE_THIS_MODULE(p4device);

/* End target device definition, contains:
 *  :p4proto definition
 *  :Pipeline definition (p4info)
 *  :Bridge and port mappings
 *  :and many other entities
 */

// TODO: Use array/hmap for multiple devices?
struct p4device {
    /* hmap nodes? */
    /* Name? */
    /* P4proto switch processing. */
    struct p4proto *p4proto;    /* P4proto switch (contains p4info) */
    struct bridge *br;          /* Bridge associated with p4device */
    /* ports */
};

// TODO: unixctl "remote" may be passed as argument later?
void
p4device_init(void)
{
    VLOG_DBG("Func called: %s", __func__);

    /* Initialize all p4device(s) */
    //TODO: One time processing for p4device structures goes here, if any?
}

void
p4device_exit(void)
{
    VLOG_DBG("Func called: %s", __func__);

    /* Disconnect all p4device(s) */
    p4proto_deinit();
}

static void
p4device_run__(void)
{
    struct p4device *p4dev;
    struct sset p4types; // Set of P4 Datapath types
    const char *type;

    VLOG_DBG("Func called: %s", __func__);

    /* Let each "P4 datapath type" do the work that it needs to do. */

    ///////////////////////////////////////////////////////////////////////
    /*1. p4proto library is correctly initialized in p4proto_init()
     * -    means all datapath types to be used are registered (class_register)
     *2. vswitchd will loop over each datapath types now and let the datapath
     * handle all the things it needs to.
     * -    such as, process port changes in this datapath, handle upcall,
     *      sending mis-matched packets to OF controller?, etc.
     *3. Datapath finishes these logics by implementing the callback type_run()
    */

    /* For Ref: ofproto-dpif is the built in datapath type for ofproto,
     * so type_run() is implemented for it. There can be other datapath
     * type implementations by registering a new class.
    */
    ///////////////////////////////////////////////////////////////////////

    /* TODO: Keep and Iterate over "set" of each of P4 datapath types  */
    /* TODO: Change function protoype, pass (p4dev->p4proto) as argument */
    p4proto_type_run();


    /* Let each "P4 device" do the work that it needs to do. */

    //////////////////////////////////////////////////////////////////////
    /*In each loop, vswitchd let each P4 device handle all its class affairs
     * first in p4proto_run(), then it can proceed to the handling of port
     * changes and Openflow msgs via connmgr, etc.
     */
    //////////////////////////////////////////////////////////////////////

    /* TODO: Iterate over HashMap/array of multiple P4 devices*/
    /* TODO: Change function protoype, pass (p4dev->p4proto) as argument */
    p4proto_run();
}

void
p4device_run(void)
{
    /* This only needs to run once, but it must be done after the
     * configuration is set.
     *
     * TODO: For now, configuration is being set within bridge_run.
     * We may need seperate configuration handling for p4device(s).
     */

    VLOG_DBG("Func called: %s", __func__);

    /* Step1: Initilaize the p4proto library*/
    p4proto_init();

    /* Step2: P4 datapath and P4 device processing*/
    p4device_run__();
}

