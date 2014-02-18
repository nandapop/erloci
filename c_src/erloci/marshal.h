/* Copyright 2012 K2Informatics GmbH, Root Laengenbold, Switzerland
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */ 
#ifndef OCI_MARSHAL_H
#define OCI_MARSHAL_H

#include <iostream>
#include <queue>
#include <vector>

using namespace std;

// Protocol spec
#define PKT_LEN_BYTES	4

// Debug Levels
#define DBG_0			0	// All Debug (Including Function Entry Exit)
#define DBG_1			1	// Binaries for Rx and Tx
#define DBG_2			2	//
#define DBG_3			3	//
#define DBG_4			4	//
#define DBG_5			5	// Debug is off

#define DEBUG			DBG_5

typedef struct _erlcmdtable {
    int cmd;
	const char * cmd_str;
    int arg_count;
    const char * cmd_description;
} erlcmdtable;

typedef enum _ERL_DEBUG {
    DBG_FLAG_OFF	= 0,
    DBG_FLAG_ON		= 1,
} ERL_DEBUG;

/* MUST use conteneous and monotonically
 * increasing number codes for new commands
 * MUST match with oci.hrl
 */
typedef enum _ERL_CMD {
    RMOTE_MSG			= 0,
    CMD_UNKWN			= 1,
    GET_SESSN			= 2,
    PUT_SESSN			= 3,
    PREP_STMT			= 4,
    BIND_ARGS			= 5,
    EXEC_STMT			= 6,
    FTCH_ROWS			= 7,
    CLSE_STMT			= 8,
    CMT_SESSN			= 9,
    RBK_SESSN			= 10,
	CMD_DSCRB			= 11,
	CMD_ECHOT			= 12,
    OCIP_QUIT			= 13
} ERL_CMD;

/*
TODO: document communication interface here
Must match with that or erlang side
*/
extern const erlcmdtable cmdtbl[];
#define CMD_NAME_STR(_cmd)		cmdtbl[((_cmd) > OCIP_QUIT ? 0 : (_cmd))].cmd_str
#define CMD_ARGS_COUNT(_cmd)	cmdtbl[((_cmd) > OCIP_QUIT ? 0 : (_cmd))].arg_count
#define CMD_DESCIPTION(_cmd)	cmdtbl[((_cmd) > OCIP_QUIT ? 0 : (_cmd))].cmd_description
#define CMDTABLE \
{\
    {RMOTE_MSG,	"RMOTE_MSG",	2, "Remote debugging turning on/off"},\
    {CMD_UNKWN,	"CMD_UNKWN",	0, "Unknown command"},\
    {GET_SESSN,	"GET_SESSN",	4, "Get a OCI session"},\
    {PUT_SESSN,	"PUT_SESSN",	2, "Release a OCI session"},\
    {PREP_STMT,	"PREP_STMT",	3, "Prepare a statement from SQL string"},\
    {BIND_ARGS,	"BIND_ARGS",	4, "Bind parameters into prepared SQL statement"},\
    {EXEC_STMT,	"EXEC_STMT",	5, "Execute a prepared statement"},\
    {FTCH_ROWS,	"FTCH_ROWS",	4, "Fetch rows from statements producing rows"},\
    {CLSE_STMT,	"CLSE_STMT",	3, "Close a statement"},\
    {CMT_SESSN,	"CMT_SESSN",	2, "Commit OCI session, starts a "},\
    {RBK_SESSN,	"RBK_SESSN",	2, "Remote debugging turning on/off"},\
    {CMD_DSCRB,	"CMD_DSCRB",	4, "Describe a DB object string"},\
    {CMD_ECHOT,	"CMD_ECHOT",	2, "Echo back erlang term"},\
    {OCIP_QUIT,	"OCIP_QUIT",	1, "Exit the port process"},\
}

extern char * print_term(void*);

#include "lib_interface.h"

// Erlang Interface Macros
#define ARG_COUNT(_command)				(erl_size(_command) - 1)
#define MAP_ARGS(_cmdcnt, _command, _target) \
{\
	(_target) = new ETERM*[(_cmdcnt)];\
	(_target)[0] = erl_element(1, _command);\
	for(int _i=1;_i<ARG_COUNT(_command); ++_i)\
		(_target)[_i] = erl_element(_i+2, _command);\
}
#define UNMAP_ARGS(_cmdcnt, _target) \
{\
	for(int _i=0;_i<(_cmdcnt); ++_i)\
		erl_free_term((_target)[_i]);\
	delete (_target);\
}

#define PRINT_ERL_ALLOC(_mark) \
{\
	unsigned long allocated=0, freed=0;\
	erl_eterm_statistics(&allocated, &freed);\
	REMOTE_LOG(_mark" eterm mem %lu, %lu\n", allocated, freed);\
}

extern bool init_marshall(void);
extern void read_cmd(void);
extern int write_resp(void * resp_term);
extern void log_args(int, void *, const char *);

#if DEBUG <= DBG_3
#define LOG_ARGS(_count,_args,_str)	    log_args((_count),(_args),(_str))
#else
#define LOG_ARGS(_count,_args,_str)
#endif

#ifdef __WIN32__
	#define lock(__mutex) (WAIT_OBJECT_0 == WaitForSingleObject((__mutex),INFINITE))
    #define unlock(__mutex) ReleaseMutex(__mutex);
#else
	#define lock(__mutex) (0 == pthread_mutex_lock(&(__mutex)))
    #define unlock(__mutex) pthread_mutex_unlock(&(__mutex))
#endif

// ThreadPool and IdleTimer
extern void InitializeThreadPool(void);
extern void CleanupThreadPool(void);
extern void ProcessCommand(void);

extern size_t calculate_resp_size(void * resp);
extern void append_list_to_list(const void * sub_list, void * list);
extern void append_int_to_list(const int integer, void * list);
extern void append_string_to_list(const char * string, size_t len, void * list);
extern void append_coldef_to_list(const char * col_name, size_t len,
								  const unsigned short data_type, const unsigned int max_len, const unsigned short precision,
								  const signed char scale, void * list);
extern void append_desc_to_list(const char * col_name, size_t len, const unsigned short data_type, const unsigned int max_len, void * list);
extern void map_schema_to_bind_args(void *, vector<var> &);
extern size_t map_value_to_bind_args(void *, vector<var> &);

#include<vector>
extern vector<unsigned char> pop_cmd_queue();

#define MAX_FORMATTED_STR_LEN 1024

// Printing the packet
#define DUMP(__tag,__len, __buf)								\
{    															\
	char *_t = new char[__len*4+1];								\
	char *_pt = _t;												\
    for(unsigned int j=0; j<__len; ++j) {						\
		sprintf(_pt,(0==(j+1)%16 ? "%02X\n" : "%02X "),			\
				(unsigned char)__buf[j]);						\
		_pt += 3;												\
    }															\
	REMOTE_LOG(DBG, "%s (%d):\n---\n%s\n---", __tag,__len, _t);	\
	delete _t;													\
}

#if DEBUG <= DBG_1
#define LOG_DUMP(__tag,__len, __buf) DUMP(__tag,__len, __buf)
#else
#define LOG_DUMP(__tag, __len, __buf)
#endif

// 32 bit packet header
typedef union _pack_hdr {
    char len_buf[4];
    unsigned long len;
} pkt_hdr;

#endif // OCI_MARSHAL_H