/*
 * Copyright (c) 2008-2017 Nicira, Inc.
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

#ifndef P4PROTO_OBJECTS_H
#define P4PROTO_OBJECTS_H 1

#include "openvswitch/list.h"

/* 1. This strucutre is a placeholder for soutbbound dpif interface
 * 2. Entries will be initialized in lib/dpif-"p4proto-type(s)".
 * 3. For our current implementation, it should be a wrapper for "P4-SDE".
 * 4. A new implementation, called "lib/dpif-p4sde" can be maintained?
 */
struct p4proto_tbl_entry {
};

#endif  /* p4proto-objects.h */
