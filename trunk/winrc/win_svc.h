/*
 * winrc/win_svc.h - windows services API implementation for unbound
 *
 * Copyright (c) 2009, NLnet Labs. All rights reserved.
 *
 * This software is open source.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * Neither the name of the NLNET LABS nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 *
 * This file contains functions to integrate with the windows services API.
 * This means it handles the commandline switches to install and remove
 * the service (via CreateService and DeleteService), it handles
 * the ServiceMain() main service entry point when started as a service,
 * and it handles the Handler[_ex]() to process requests to the service
 * (such as start and stop and status).
 */

#ifndef WINRC_WIN_SVC_H
#define WINRC_WIN_SVC_H
struct worker;

/** service name for unbound (internal to ServiceManager) */
#define SERVICE_NAME "unbound"

/**
 * Handle commandline service for windows.
 * @param wopt: windows option string (install, remove, service). 
 * @param cfgfile: configfile to open (default or passed with -c).
 * @param v: amount of commandline verbosity added with -v.
 */
void wsvc_command_option(const char* wopt, const char* cfgfile, int v);

/**
 * Setup lead worker events.
 * @param worker: the worker
 */
void wsvc_setup_worker(struct worker* worker);

/** windows worker stop event callback handler */
void worker_win_stop_cb(int fd, short ev, void* arg);

#endif /* WINRC_WIN_SVC_H */
