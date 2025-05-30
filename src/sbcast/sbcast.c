/*****************************************************************************\
 *  sbcast.c - Broadcast a file to allocated nodes
 *****************************************************************************
 *  Copyright (C) 2006-2007 The Regents of the University of California.
 *  Copyright (C) 2008-2009 Lawrence Livermore National Security.
 *  Copyright (C) SchedMD LLC.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Morris Jette <jette1@llnl.gov>
 *  CODE-OCEC-09-009. All rights reserved.
 *
 *  This file is part of Slurm, a resource management program.
 *  For details, see <https://slurm.schedmd.com/>.
 *  Please also read the included file: DISCLAIMER.
 *
 *  Slurm is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission
 *  to link the code of portions of this program with the OpenSSL library under
 *  certain conditions as described in each individual source file, and
 *  distribute linked combinations including the two. You must obey the GNU
 *  General Public License in all respects for all of the code used other than
 *  OpenSSL. If you modify file(s) with this exception, you may extend this
 *  exception to your version of the file(s), but you are not obligated to do
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in
 *  the program, then also delete it here.
 *
 *  Slurm is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Slurm; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
\*****************************************************************************/

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "slurm/slurm_errno.h"

#include "src/common/forward.h"
#include "src/common/hostlist.h"
#include "src/common/log.h"
#include "src/common/read_config.h"
#include "src/sbcast/sbcast.h"
#include "src/interfaces/cred.h"
#include "src/common/slurm_protocol_api.h"
#include "src/common/slurm_protocol_socket.h"
#include "src/common/slurm_time.h"
#include "src/common/uid.h"
#include "src/common/xmalloc.h"
#include "src/common/xstring.h"

/* global variables */
struct bcast_parameters params = {0};	/* program parameters */

static int _foreach_bcast_file(void *x, void *arg)
{
	int *rc = arg;
	params.selected_step = x;

	if ((*rc = bcast_file(&params) != SLURM_SUCCESS)) {
		error("Failed to broadcast to JobId=%u nodes",
		      params.selected_step->step_id.job_id);
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	int rc;
	log_options_t opts = LOG_OPTS_STDERR_ONLY;
	log_init("sbcast", opts, SYSLOG_FACILITY_DAEMON, NULL);

	slurm_init(NULL);

	parse_command_line(argc, argv);
	if (params.verbose) {
		opts.stderr_level += params.verbose;
		log_alter(opts, SYSLOG_FACILITY_DAEMON, NULL);
	}

	if (params.selected_steps) {
		slurm_destroy_selected_step(params.selected_step);
		list_for_each_ro(params.selected_steps, _foreach_bcast_file,
				 &rc);
		FREE_NULL_LIST(params.selected_steps);
	} else {
		rc = bcast_file(&params);
	}

	return rc;
}
