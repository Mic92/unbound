%{
/*
 * configlexer.lex - lexical analyzer for unbound config file
 *
 * Copyright (c) 2001-2006, NLnet Labs. All rights reserved
 *
 * See LICENSE for the license.
 *
 */

#include "config.h"

#include <ctype.h>
#include <string.h>
#include <strings.h>

#include "util/configyyrename.h"
#include "util/config_file.h"
#include "util/configparser.h"
void ub_c_error(const char *message);

#if 0
#define LEXOUT(s)  printf s /* used ONLY when debugging */
#else
#define LEXOUT(s)
#endif

/** avoid warning in about fwrite return value */
#define ECHO ub_c_error_msg("syntax error at text: %s", yytext)

/** A parser variable, this is a statement in the config file which is
 * of the form variable: value1 value2 ...  nargs is the number of values. */
#define YDVAR(nargs, var) \
	num_args=(nargs); \
	LEXOUT(("v(%s%d) ", yytext, num_args)); \
	if(num_args > 0) { BEGIN(val); } \
	return (var);

struct inc_state {
	char* filename;
	int line;
};
static struct inc_state parse_stack[MAXINCLUDES];
static YY_BUFFER_STATE include_stack[MAXINCLUDES];
static int config_include_stack_ptr = 0;
static int inc_prev = 0;
static int num_args = 0;

static void config_start_include(const char* filename)
{
	FILE *input;
	if(strlen(filename) == 0) {
		ub_c_error_msg("empty include file name");
		return;
	}
	if(config_include_stack_ptr >= MAXINCLUDES) {
		ub_c_error_msg("includes nested too deeply, skipped (>%d)", MAXINCLUDES);
		return;
	}
	if(cfg_parser->chroot && strncmp(filename, cfg_parser->chroot,
		strlen(cfg_parser->chroot)) == 0) {
		filename += strlen(cfg_parser->chroot);
	}
	input = fopen(filename, "r");
	if(!input) {
		ub_c_error_msg("cannot open include file '%s': %s",
			filename, strerror(errno));
		return;
	}
	LEXOUT(("switch_to_include_file(%s) ", filename));
	parse_stack[config_include_stack_ptr].filename = cfg_parser->filename;
	parse_stack[config_include_stack_ptr].line = cfg_parser->line;
	include_stack[config_include_stack_ptr] = YY_CURRENT_BUFFER;
	cfg_parser->filename = strdup(filename);
	cfg_parser->line = 1;
	yy_switch_to_buffer(yy_create_buffer(input, YY_BUF_SIZE));
	++config_include_stack_ptr;
}

static void config_end_include(void)
{
	--config_include_stack_ptr;
	free(cfg_parser->filename);
	cfg_parser->filename = parse_stack[config_include_stack_ptr].filename;
	cfg_parser->line = parse_stack[config_include_stack_ptr].line;
	yy_delete_buffer(YY_CURRENT_BUFFER);
	yy_switch_to_buffer(include_stack[config_include_stack_ptr]);
}

#ifndef yy_set_bol /* compat definition, for flex 2.4.6 */
#define yy_set_bol(at_bol) \
        { \
	        if ( ! yy_current_buffer ) \
	                yy_current_buffer = yy_create_buffer( yyin, YY_BUF_SIZE ); \
	        yy_current_buffer->yy_ch_buf[0] = ((at_bol)?'\n':' '); \
        }
#endif

%}
%option noinput
%option nounput
%{
#ifndef YY_NO_UNPUT
#define YY_NO_UNPUT 1
#endif
#ifndef YY_NO_INPUT
#define YY_NO_INPUT 1
#endif
%}

SPACE   [ \t]
LETTER  [a-zA-Z]
UNQUOTEDLETTER [^\'\"\n\r \t\\]|\\.
UNQUOTEDLETTER_NOCOLON [^\:\'\"\n\r \t\\]|\\.
NEWLINE [\r\n]
COMMENT \#
COLON 	\:
DQANY     [^\"\n\r\\]|\\.
SQANY     [^\'\n\r\\]|\\.

%x	quotedstring singlequotedstr include include_quoted val

%%
<INITIAL,val>{SPACE}*	{ 
	LEXOUT(("SP ")); /* ignore */ }
<INITIAL,val>{SPACE}*{COMMENT}.*$	{ 
	LEXOUT(("comment(%s) ", yytext)); /* ignore */ }
server{COLON}			{ YDVAR(0, VAR_SERVER) }
num-threads{COLON}		{ YDVAR(1, VAR_NUM_THREADS) }
verbosity{COLON}		{ YDVAR(1, VAR_VERBOSITY) }
port{COLON}			{ YDVAR(1, VAR_PORT) }
outgoing-range{COLON}		{ YDVAR(1, VAR_OUTGOING_RANGE) }
outgoing-port-permit{COLON}	{ YDVAR(1, VAR_OUTGOING_PORT_PERMIT) }
outgoing-port-avoid{COLON}	{ YDVAR(1, VAR_OUTGOING_PORT_AVOID) }
outgoing-num-tcp{COLON}		{ YDVAR(1, VAR_OUTGOING_NUM_TCP) }
incoming-num-tcp{COLON}		{ YDVAR(1, VAR_INCOMING_NUM_TCP) }
do-ip4{COLON}			{ YDVAR(1, VAR_DO_IP4) }
do-ip6{COLON}			{ YDVAR(1, VAR_DO_IP6) }
do-udp{COLON}			{ YDVAR(1, VAR_DO_UDP) }
do-tcp{COLON}			{ YDVAR(1, VAR_DO_TCP) }
do-daemonize{COLON}		{ YDVAR(1, VAR_DO_DAEMONIZE) }
interface{COLON}		{ YDVAR(1, VAR_INTERFACE) }
outgoing-interface{COLON}	{ YDVAR(1, VAR_OUTGOING_INTERFACE) }
interface-automatic{COLON}	{ YDVAR(1, VAR_INTERFACE_AUTOMATIC) }
chroot{COLON}			{ YDVAR(1, VAR_CHROOT) }
username{COLON}			{ YDVAR(1, VAR_USERNAME) }
directory{COLON}		{ YDVAR(1, VAR_DIRECTORY) }
logfile{COLON}			{ YDVAR(1, VAR_LOGFILE) }
pidfile{COLON}			{ YDVAR(1, VAR_PIDFILE) }
root-hints{COLON}		{ YDVAR(1, VAR_ROOT_HINTS) }
msg-buffer-size{COLON}		{ YDVAR(1, VAR_MSG_BUFFER_SIZE) }
msg-cache-size{COLON}		{ YDVAR(1, VAR_MSG_CACHE_SIZE) }
msg-cache-slabs{COLON}		{ YDVAR(1, VAR_MSG_CACHE_SLABS) }
rrset-cache-size{COLON}		{ YDVAR(1, VAR_RRSET_CACHE_SIZE) }
rrset-cache-slabs{COLON}	{ YDVAR(1, VAR_RRSET_CACHE_SLABS) }
cache-max-ttl{COLON}     	{ YDVAR(1, VAR_CACHE_MAX_TTL) }
cache-min-ttl{COLON}     	{ YDVAR(1, VAR_CACHE_MIN_TTL) }
infra-host-ttl{COLON}		{ YDVAR(1, VAR_INFRA_HOST_TTL) }
infra-lame-ttl{COLON}		{ YDVAR(1, VAR_INFRA_LAME_TTL) }
infra-cache-slabs{COLON}	{ YDVAR(1, VAR_INFRA_CACHE_SLABS) }
infra-cache-numhosts{COLON}	{ YDVAR(1, VAR_INFRA_CACHE_NUMHOSTS) }
infra-cache-lame-size{COLON}	{ YDVAR(1, VAR_INFRA_CACHE_LAME_SIZE) }
num-queries-per-thread{COLON}	{ YDVAR(1, VAR_NUM_QUERIES_PER_THREAD) }
jostle-timeout{COLON}		{ YDVAR(1, VAR_JOSTLE_TIMEOUT) }
target-fetch-policy{COLON}	{ YDVAR(1, VAR_TARGET_FETCH_POLICY) }
harden-short-bufsize{COLON}	{ YDVAR(1, VAR_HARDEN_SHORT_BUFSIZE) }
harden-large-queries{COLON}	{ YDVAR(1, VAR_HARDEN_LARGE_QUERIES) }
harden-glue{COLON}		{ YDVAR(1, VAR_HARDEN_GLUE) }
harden-dnssec-stripped{COLON}	{ YDVAR(1, VAR_HARDEN_DNNSEC_STRIPPED) }
harden-referral-path{COLON}	{ YDVAR(1, VAR_HARDEN_REFERRAL_PATH) }
use-caps-for-id{COLON}		{ YDVAR(1, VAR_USE_CAPS_FOR_ID) }
unwanted-reply-threshold{COLON}	{ YDVAR(1, VAR_UNWANTED_REPLY_THRESHOLD) }
private-address{COLON}		{ YDVAR(1, VAR_PRIVATE_ADDRESS) }
private-domain{COLON}		{ YDVAR(1, VAR_PRIVATE_DOMAIN) }
stub-zone{COLON}		{ YDVAR(0, VAR_STUB_ZONE) }
name{COLON}			{ YDVAR(1, VAR_NAME) }
stub-addr{COLON}		{ YDVAR(1, VAR_STUB_ADDR) }
stub-host{COLON}		{ YDVAR(1, VAR_STUB_HOST) }
stub-prime{COLON}		{ YDVAR(1, VAR_STUB_PRIME) }
forward-zone{COLON}		{ YDVAR(0, VAR_FORWARD_ZONE) }
forward-addr{COLON}		{ YDVAR(1, VAR_FORWARD_ADDR) }
forward-host{COLON}		{ YDVAR(1, VAR_FORWARD_HOST) }
do-not-query-address{COLON}	{ YDVAR(1, VAR_DO_NOT_QUERY_ADDRESS) }
do-not-query-localhost{COLON}	{ YDVAR(1, VAR_DO_NOT_QUERY_LOCALHOST) }
access-control{COLON}		{ YDVAR(2, VAR_ACCESS_CONTROL) }
hide-identity{COLON}		{ YDVAR(1, VAR_HIDE_IDENTITY) }
hide-version{COLON}		{ YDVAR(1, VAR_HIDE_VERSION) }
identity{COLON}			{ YDVAR(1, VAR_IDENTITY) }
version{COLON}			{ YDVAR(1, VAR_VERSION) }
module-config{COLON}     	{ YDVAR(1, VAR_MODULE_CONF) }
dlv-anchor{COLON}		{ YDVAR(1, VAR_DLV_ANCHOR) }
dlv-anchor-file{COLON}		{ YDVAR(1, VAR_DLV_ANCHOR_FILE) }
trust-anchor-file{COLON}	{ YDVAR(1, VAR_TRUST_ANCHOR_FILE) }
trusted-keys-file{COLON}	{ YDVAR(1, VAR_TRUSTED_KEYS_FILE) }
trust-anchor{COLON}		{ YDVAR(1, VAR_TRUST_ANCHOR) }
val-override-date{COLON}	{ YDVAR(1, VAR_VAL_OVERRIDE_DATE) }
val-sig-skew-min{COLON}		{ YDVAR(1, VAR_VAL_SIG_SKEW_MIN) }
val-sig-skew-max{COLON}		{ YDVAR(1, VAR_VAL_SIG_SKEW_MAX) }
val-bogus-ttl{COLON}		{ YDVAR(1, VAR_BOGUS_TTL) }
val-clean-additional{COLON}	{ YDVAR(1, VAR_VAL_CLEAN_ADDITIONAL) }
val-permissive-mode{COLON}	{ YDVAR(1, VAR_VAL_PERMISSIVE_MODE) }
key-cache-size{COLON}		{ YDVAR(1, VAR_KEY_CACHE_SIZE) }
key-cache-slabs{COLON}		{ YDVAR(1, VAR_KEY_CACHE_SLABS) }
neg-cache-size{COLON}		{ YDVAR(1, VAR_NEG_CACHE_SIZE) }
val-nsec3-keysize-iterations{COLON}	{ 
				  YDVAR(1, VAR_VAL_NSEC3_KEYSIZE_ITERATIONS) }
use-syslog{COLON}		{ YDVAR(1, VAR_USE_SYSLOG) }
log-time-ascii{COLON}		{ YDVAR(1, VAR_LOG_TIME_ASCII) }
local-zone{COLON}		{ YDVAR(2, VAR_LOCAL_ZONE) }
local-data{COLON}		{ YDVAR(1, VAR_LOCAL_DATA) }
local-data-ptr{COLON}		{ YDVAR(1, VAR_LOCAL_DATA_PTR) }
statistics-interval{COLON}	{ YDVAR(1, VAR_STATISTICS_INTERVAL) }
statistics-cumulative{COLON}	{ YDVAR(1, VAR_STATISTICS_CUMULATIVE) }
extended-statistics{COLON}	{ YDVAR(1, VAR_EXTENDED_STATISTICS) }
remote-control{COLON}		{ YDVAR(0, VAR_REMOTE_CONTROL) }
control-enable{COLON}		{ YDVAR(1, VAR_CONTROL_ENABLE) }
control-interface{COLON}	{ YDVAR(1, VAR_CONTROL_INTERFACE) }
control-port{COLON}		{ YDVAR(1, VAR_CONTROL_PORT) }
server-key-file{COLON}		{ YDVAR(1, VAR_SERVER_KEY_FILE) }
server-cert-file{COLON}		{ YDVAR(1, VAR_SERVER_CERT_FILE) }
control-key-file{COLON}		{ YDVAR(1, VAR_CONTROL_KEY_FILE) }
control-cert-file{COLON}	{ YDVAR(1, VAR_CONTROL_CERT_FILE) }
python-script{COLON}		{ YDVAR(1, VAR_PYTHON_SCRIPT) }
python{COLON}			{ YDVAR(0, VAR_PYTHON) }
domain-insecure{COLON}		{ YDVAR(1, VAR_DOMAIN_INSECURE) }
<INITIAL,val>{NEWLINE}		{ LEXOUT(("NL\n")); cfg_parser->line++; }

	/* Quoted strings. Strip leading and ending quotes */
<val>\"			{ BEGIN(quotedstring); LEXOUT(("QS ")); }
<quotedstring><<EOF>>   {
        yyerror("EOF inside quoted string");
	if(--num_args == 0) { BEGIN(INITIAL); }
	else		    { BEGIN(val); }
}
<quotedstring>{DQANY}*  { LEXOUT(("STR(%s) ", yytext)); yymore(); }
<quotedstring>{NEWLINE} { yyerror("newline inside quoted string, no end \""); 
			  cfg_parser->line++; BEGIN(INITIAL); }
<quotedstring>\" {
        LEXOUT(("QE "));
	if(--num_args == 0) { BEGIN(INITIAL); }
	else		    { BEGIN(val); }
        yytext[yyleng - 1] = '\0';
	yylval.str = strdup(yytext);
	if(!yylval.str)
		yyerror("out of memory");
        return STRING_ARG;
}

	/* Single Quoted strings. Strip leading and ending quotes */
<val>\'			{ BEGIN(singlequotedstr); LEXOUT(("SQS ")); }
<singlequotedstr><<EOF>>   {
        yyerror("EOF inside quoted string");
	if(--num_args == 0) { BEGIN(INITIAL); }
	else		    { BEGIN(val); }
}
<singlequotedstr>{SQANY}*  { LEXOUT(("STR(%s) ", yytext)); yymore(); }
<singlequotedstr>{NEWLINE} { yyerror("newline inside quoted string, no end '"); 
			     cfg_parser->line++; BEGIN(INITIAL); }
<singlequotedstr>\' {
        LEXOUT(("SQE "));
	if(--num_args == 0) { BEGIN(INITIAL); }
	else		    { BEGIN(val); }
        yytext[yyleng - 1] = '\0';
	yylval.str = strdup(yytext);
	if(!yylval.str)
		yyerror("out of memory");
        return STRING_ARG;
}

	/* include: directive */
<INITIAL,val>include{COLON}	{ 
	LEXOUT(("v(%s) ", yytext)); inc_prev = YYSTATE; BEGIN(include); }
<include><<EOF>>	{
        yyerror("EOF inside include directive");
        BEGIN(inc_prev);
}
<include>{SPACE}*	{ LEXOUT(("ISP ")); /* ignore */ }
<include>{NEWLINE}	{ LEXOUT(("NL\n")); cfg_parser->line++;}
<include>\"		{ LEXOUT(("IQS ")); BEGIN(include_quoted); }
<include>{UNQUOTEDLETTER}*	{
	LEXOUT(("Iunquotedstr(%s) ", yytext));
	config_start_include(yytext);
	BEGIN(inc_prev);
}
<include_quoted><<EOF>>	{
        yyerror("EOF inside quoted string");
        BEGIN(inc_prev);
}
<include_quoted>{DQANY}*	{ LEXOUT(("ISTR(%s) ", yytext)); yymore(); }
<include_quoted>{NEWLINE}	{ yyerror("newline before \" in include name"); 
				  cfg_parser->line++; BEGIN(inc_prev); }
<include_quoted>\"	{
	LEXOUT(("IQE "));
	yytext[yyleng - 1] = '\0';
	config_start_include(yytext);
	BEGIN(inc_prev);
}
<INITIAL,val><<EOF>>	{
	yy_set_bol(1); /* Set beginning of line, so "^" rules match.  */
	if (config_include_stack_ptr == 0) {
		yyterminate();
	} else {
		fclose(yyin);
		config_end_include();
	}
}

<val>{UNQUOTEDLETTER}*	{ LEXOUT(("unquotedstr(%s) ", yytext)); 
			if(--num_args == 0) { BEGIN(INITIAL); }
			yylval.str = strdup(yytext); return STRING_ARG; }

{UNQUOTEDLETTER_NOCOLON}*	{
	ub_c_error_msg("unknown keyword '%s'", yytext);
	}

<*>.	{
	ub_c_error_msg("stray '%s'", yytext);
	}

%%
