/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_UTIL_CONFIGPARSER_H_INCLUDED
# define YY_YY_UTIL_CONFIGPARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    SPACE = 258,
    LETTER = 259,
    NEWLINE = 260,
    COMMENT = 261,
    COLON = 262,
    ANY = 263,
    ZONESTR = 264,
    STRING_ARG = 265,
    VAR_SERVER = 266,
    VAR_VERBOSITY = 267,
    VAR_NUM_THREADS = 268,
    VAR_PORT = 269,
    VAR_OUTGOING_RANGE = 270,
    VAR_INTERFACE = 271,
    VAR_DO_IP4 = 272,
    VAR_DO_IP6 = 273,
    VAR_DO_UDP = 274,
    VAR_DO_TCP = 275,
    VAR_CHROOT = 276,
    VAR_USERNAME = 277,
    VAR_DIRECTORY = 278,
    VAR_LOGFILE = 279,
    VAR_PIDFILE = 280,
    VAR_MSG_CACHE_SIZE = 281,
    VAR_MSG_CACHE_SLABS = 282,
    VAR_NUM_QUERIES_PER_THREAD = 283,
    VAR_RRSET_CACHE_SIZE = 284,
    VAR_RRSET_CACHE_SLABS = 285,
    VAR_OUTGOING_NUM_TCP = 286,
    VAR_INFRA_HOST_TTL = 287,
    VAR_INFRA_LAME_TTL = 288,
    VAR_INFRA_CACHE_SLABS = 289,
    VAR_INFRA_CACHE_NUMHOSTS = 290,
    VAR_INFRA_CACHE_LAME_SIZE = 291,
    VAR_NAME = 292,
    VAR_STUB_ZONE = 293,
    VAR_STUB_HOST = 294,
    VAR_STUB_ADDR = 295,
    VAR_TARGET_FETCH_POLICY = 296,
    VAR_HARDEN_SHORT_BUFSIZE = 297,
    VAR_HARDEN_LARGE_QUERIES = 298,
    VAR_FORWARD_ZONE = 299,
    VAR_FORWARD_HOST = 300,
    VAR_FORWARD_ADDR = 301,
    VAR_DO_NOT_QUERY_ADDRESS = 302,
    VAR_HIDE_IDENTITY = 303,
    VAR_HIDE_VERSION = 304,
    VAR_IDENTITY = 305,
    VAR_VERSION = 306,
    VAR_HARDEN_GLUE = 307,
    VAR_MODULE_CONF = 308,
    VAR_TRUST_ANCHOR_FILE = 309,
    VAR_TRUST_ANCHOR = 310,
    VAR_VAL_OVERRIDE_DATE = 311,
    VAR_BOGUS_TTL = 312,
    VAR_VAL_CLEAN_ADDITIONAL = 313,
    VAR_VAL_PERMISSIVE_MODE = 314,
    VAR_INCOMING_NUM_TCP = 315,
    VAR_MSG_BUFFER_SIZE = 316,
    VAR_KEY_CACHE_SIZE = 317,
    VAR_KEY_CACHE_SLABS = 318,
    VAR_TRUSTED_KEYS_FILE = 319,
    VAR_VAL_NSEC3_KEYSIZE_ITERATIONS = 320,
    VAR_USE_SYSLOG = 321,
    VAR_OUTGOING_INTERFACE = 322,
    VAR_ROOT_HINTS = 323,
    VAR_DO_NOT_QUERY_LOCALHOST = 324,
    VAR_CACHE_MAX_TTL = 325,
    VAR_HARDEN_DNSSEC_STRIPPED = 326,
    VAR_ACCESS_CONTROL = 327,
    VAR_LOCAL_ZONE = 328,
    VAR_LOCAL_DATA = 329,
    VAR_INTERFACE_AUTOMATIC = 330,
    VAR_STATISTICS_INTERVAL = 331,
    VAR_DO_DAEMONIZE = 332,
    VAR_USE_CAPS_FOR_ID = 333,
    VAR_STATISTICS_CUMULATIVE = 334,
    VAR_OUTGOING_PORT_PERMIT = 335,
    VAR_OUTGOING_PORT_AVOID = 336,
    VAR_DLV_ANCHOR_FILE = 337,
    VAR_DLV_ANCHOR = 338,
    VAR_NEG_CACHE_SIZE = 339,
    VAR_HARDEN_REFERRAL_PATH = 340,
    VAR_PRIVATE_ADDRESS = 341,
    VAR_PRIVATE_DOMAIN = 342,
    VAR_REMOTE_CONTROL = 343,
    VAR_CONTROL_ENABLE = 344,
    VAR_CONTROL_INTERFACE = 345,
    VAR_CONTROL_PORT = 346,
    VAR_SERVER_KEY_FILE = 347,
    VAR_SERVER_CERT_FILE = 348,
    VAR_CONTROL_KEY_FILE = 349,
    VAR_CONTROL_CERT_FILE = 350,
    VAR_CONTROL_USE_CERT = 351,
    VAR_EXTENDED_STATISTICS = 352,
    VAR_LOCAL_DATA_PTR = 353,
    VAR_JOSTLE_TIMEOUT = 354,
    VAR_STUB_PRIME = 355,
    VAR_UNWANTED_REPLY_THRESHOLD = 356,
    VAR_LOG_TIME_ASCII = 357,
    VAR_DOMAIN_INSECURE = 358,
    VAR_PYTHON = 359,
    VAR_PYTHON_SCRIPT = 360,
    VAR_VAL_SIG_SKEW_MIN = 361,
    VAR_VAL_SIG_SKEW_MAX = 362,
    VAR_CACHE_MIN_TTL = 363,
    VAR_VAL_LOG_LEVEL = 364,
    VAR_AUTO_TRUST_ANCHOR_FILE = 365,
    VAR_KEEP_MISSING = 366,
    VAR_ADD_HOLDDOWN = 367,
    VAR_DEL_HOLDDOWN = 368,
    VAR_SO_RCVBUF = 369,
    VAR_EDNS_BUFFER_SIZE = 370,
    VAR_PREFETCH = 371,
    VAR_PREFETCH_KEY = 372,
    VAR_SO_SNDBUF = 373,
    VAR_SO_REUSEPORT = 374,
    VAR_HARDEN_BELOW_NXDOMAIN = 375,
    VAR_IGNORE_CD_FLAG = 376,
    VAR_LOG_QUERIES = 377,
    VAR_TCP_UPSTREAM = 378,
    VAR_SSL_UPSTREAM = 379,
    VAR_SSL_SERVICE_KEY = 380,
    VAR_SSL_SERVICE_PEM = 381,
    VAR_SSL_PORT = 382,
    VAR_FORWARD_FIRST = 383,
    VAR_STUB_FIRST = 384,
    VAR_MINIMAL_RESPONSES = 385,
    VAR_RRSET_ROUNDROBIN = 386,
    VAR_MAX_UDP_SIZE = 387,
    VAR_DELAY_CLOSE = 388,
    VAR_UNBLOCK_LAN_ZONES = 389,
    VAR_INFRA_CACHE_MIN_RTT = 390,
    VAR_DNS64_PREFIX = 391,
    VAR_DNS64_SYNTHALL = 392,
    VAR_DNSTAP = 393,
    VAR_DNSTAP_ENABLE = 394,
    VAR_DNSTAP_SOCKET_PATH = 395,
    VAR_DNSTAP_SEND_IDENTITY = 396,
    VAR_DNSTAP_SEND_VERSION = 397,
    VAR_DNSTAP_IDENTITY = 398,
    VAR_DNSTAP_VERSION = 399,
    VAR_DNSTAP_LOG_RESOLVER_QUERY_MESSAGES = 400,
    VAR_DNSTAP_LOG_RESOLVER_RESPONSE_MESSAGES = 401,
    VAR_DNSTAP_LOG_CLIENT_QUERY_MESSAGES = 402,
    VAR_DNSTAP_LOG_CLIENT_RESPONSE_MESSAGES = 403,
    VAR_DNSTAP_LOG_FORWARDER_QUERY_MESSAGES = 404,
    VAR_DNSTAP_LOG_FORWARDER_RESPONSE_MESSAGES = 405,
    VAR_HARDEN_ALGO_DOWNGRADE = 406,
    VAR_IP_TRANSPARENT = 407,
    VAR_RATELIMIT = 408,
    VAR_RATELIMIT_SLABS = 409,
    VAR_RATELIMIT_SIZE = 410,
    VAR_RATELIMIT_FOR_DOMAIN = 411,
    VAR_RATELIMIT_BELOW_DOMAIN = 412,
    VAR_RATELIMIT_FACTOR = 413,
    VAR_SEND_CLIENT_SUBNET = 414,
    VAR_CLIENT_SUBNET_OPCODE = 415,
    VAR_MAX_CLIENT_SUBNET_IPV4 = 416,
    VAR_MAX_CLIENT_SUBNET_IPV6 = 417
  };
#endif
/* Tokens.  */
#define SPACE 258
#define LETTER 259
#define NEWLINE 260
#define COMMENT 261
#define COLON 262
#define ANY 263
#define ZONESTR 264
#define STRING_ARG 265
#define VAR_SERVER 266
#define VAR_VERBOSITY 267
#define VAR_NUM_THREADS 268
#define VAR_PORT 269
#define VAR_OUTGOING_RANGE 270
#define VAR_INTERFACE 271
#define VAR_DO_IP4 272
#define VAR_DO_IP6 273
#define VAR_DO_UDP 274
#define VAR_DO_TCP 275
#define VAR_CHROOT 276
#define VAR_USERNAME 277
#define VAR_DIRECTORY 278
#define VAR_LOGFILE 279
#define VAR_PIDFILE 280
#define VAR_MSG_CACHE_SIZE 281
#define VAR_MSG_CACHE_SLABS 282
#define VAR_NUM_QUERIES_PER_THREAD 283
#define VAR_RRSET_CACHE_SIZE 284
#define VAR_RRSET_CACHE_SLABS 285
#define VAR_OUTGOING_NUM_TCP 286
#define VAR_INFRA_HOST_TTL 287
#define VAR_INFRA_LAME_TTL 288
#define VAR_INFRA_CACHE_SLABS 289
#define VAR_INFRA_CACHE_NUMHOSTS 290
#define VAR_INFRA_CACHE_LAME_SIZE 291
#define VAR_NAME 292
#define VAR_STUB_ZONE 293
#define VAR_STUB_HOST 294
#define VAR_STUB_ADDR 295
#define VAR_TARGET_FETCH_POLICY 296
#define VAR_HARDEN_SHORT_BUFSIZE 297
#define VAR_HARDEN_LARGE_QUERIES 298
#define VAR_FORWARD_ZONE 299
#define VAR_FORWARD_HOST 300
#define VAR_FORWARD_ADDR 301
#define VAR_DO_NOT_QUERY_ADDRESS 302
#define VAR_HIDE_IDENTITY 303
#define VAR_HIDE_VERSION 304
#define VAR_IDENTITY 305
#define VAR_VERSION 306
#define VAR_HARDEN_GLUE 307
#define VAR_MODULE_CONF 308
#define VAR_TRUST_ANCHOR_FILE 309
#define VAR_TRUST_ANCHOR 310
#define VAR_VAL_OVERRIDE_DATE 311
#define VAR_BOGUS_TTL 312
#define VAR_VAL_CLEAN_ADDITIONAL 313
#define VAR_VAL_PERMISSIVE_MODE 314
#define VAR_INCOMING_NUM_TCP 315
#define VAR_MSG_BUFFER_SIZE 316
#define VAR_KEY_CACHE_SIZE 317
#define VAR_KEY_CACHE_SLABS 318
#define VAR_TRUSTED_KEYS_FILE 319
#define VAR_VAL_NSEC3_KEYSIZE_ITERATIONS 320
#define VAR_USE_SYSLOG 321
#define VAR_OUTGOING_INTERFACE 322
#define VAR_ROOT_HINTS 323
#define VAR_DO_NOT_QUERY_LOCALHOST 324
#define VAR_CACHE_MAX_TTL 325
#define VAR_HARDEN_DNSSEC_STRIPPED 326
#define VAR_ACCESS_CONTROL 327
#define VAR_LOCAL_ZONE 328
#define VAR_LOCAL_DATA 329
#define VAR_INTERFACE_AUTOMATIC 330
#define VAR_STATISTICS_INTERVAL 331
#define VAR_DO_DAEMONIZE 332
#define VAR_USE_CAPS_FOR_ID 333
#define VAR_STATISTICS_CUMULATIVE 334
#define VAR_OUTGOING_PORT_PERMIT 335
#define VAR_OUTGOING_PORT_AVOID 336
#define VAR_DLV_ANCHOR_FILE 337
#define VAR_DLV_ANCHOR 338
#define VAR_NEG_CACHE_SIZE 339
#define VAR_HARDEN_REFERRAL_PATH 340
#define VAR_PRIVATE_ADDRESS 341
#define VAR_PRIVATE_DOMAIN 342
#define VAR_REMOTE_CONTROL 343
#define VAR_CONTROL_ENABLE 344
#define VAR_CONTROL_INTERFACE 345
#define VAR_CONTROL_PORT 346
#define VAR_SERVER_KEY_FILE 347
#define VAR_SERVER_CERT_FILE 348
#define VAR_CONTROL_KEY_FILE 349
#define VAR_CONTROL_CERT_FILE 350
#define VAR_CONTROL_USE_CERT 351
#define VAR_EXTENDED_STATISTICS 352
#define VAR_LOCAL_DATA_PTR 353
#define VAR_JOSTLE_TIMEOUT 354
#define VAR_STUB_PRIME 355
#define VAR_UNWANTED_REPLY_THRESHOLD 356
#define VAR_LOG_TIME_ASCII 357
#define VAR_DOMAIN_INSECURE 358
#define VAR_PYTHON 359
#define VAR_PYTHON_SCRIPT 360
#define VAR_VAL_SIG_SKEW_MIN 361
#define VAR_VAL_SIG_SKEW_MAX 362
#define VAR_CACHE_MIN_TTL 363
#define VAR_VAL_LOG_LEVEL 364
#define VAR_AUTO_TRUST_ANCHOR_FILE 365
#define VAR_KEEP_MISSING 366
#define VAR_ADD_HOLDDOWN 367
#define VAR_DEL_HOLDDOWN 368
#define VAR_SO_RCVBUF 369
#define VAR_EDNS_BUFFER_SIZE 370
#define VAR_PREFETCH 371
#define VAR_PREFETCH_KEY 372
#define VAR_SO_SNDBUF 373
#define VAR_SO_REUSEPORT 374
#define VAR_HARDEN_BELOW_NXDOMAIN 375
#define VAR_IGNORE_CD_FLAG 376
#define VAR_LOG_QUERIES 377
#define VAR_TCP_UPSTREAM 378
#define VAR_SSL_UPSTREAM 379
#define VAR_SSL_SERVICE_KEY 380
#define VAR_SSL_SERVICE_PEM 381
#define VAR_SSL_PORT 382
#define VAR_FORWARD_FIRST 383
#define VAR_STUB_FIRST 384
#define VAR_MINIMAL_RESPONSES 385
#define VAR_RRSET_ROUNDROBIN 386
#define VAR_MAX_UDP_SIZE 387
#define VAR_DELAY_CLOSE 388
#define VAR_UNBLOCK_LAN_ZONES 389
#define VAR_INFRA_CACHE_MIN_RTT 390
#define VAR_DNS64_PREFIX 391
#define VAR_DNS64_SYNTHALL 392
#define VAR_DNSTAP 393
#define VAR_DNSTAP_ENABLE 394
#define VAR_DNSTAP_SOCKET_PATH 395
#define VAR_DNSTAP_SEND_IDENTITY 396
#define VAR_DNSTAP_SEND_VERSION 397
#define VAR_DNSTAP_IDENTITY 398
#define VAR_DNSTAP_VERSION 399
#define VAR_DNSTAP_LOG_RESOLVER_QUERY_MESSAGES 400
#define VAR_DNSTAP_LOG_RESOLVER_RESPONSE_MESSAGES 401
#define VAR_DNSTAP_LOG_CLIENT_QUERY_MESSAGES 402
#define VAR_DNSTAP_LOG_CLIENT_RESPONSE_MESSAGES 403
#define VAR_DNSTAP_LOG_FORWARDER_QUERY_MESSAGES 404
#define VAR_DNSTAP_LOG_FORWARDER_RESPONSE_MESSAGES 405
#define VAR_HARDEN_ALGO_DOWNGRADE 406
#define VAR_IP_TRANSPARENT 407
#define VAR_RATELIMIT 408
#define VAR_RATELIMIT_SLABS 409
#define VAR_RATELIMIT_SIZE 410
#define VAR_RATELIMIT_FOR_DOMAIN 411
#define VAR_RATELIMIT_BELOW_DOMAIN 412
#define VAR_RATELIMIT_FACTOR 413
#define VAR_SEND_CLIENT_SUBNET 414
#define VAR_CLIENT_SUBNET_OPCODE 415
#define VAR_MAX_CLIENT_SUBNET_IPV4 416
#define VAR_MAX_CLIENT_SUBNET_IPV6 417

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 64 "./util/configparser.y" /* yacc.c:1909  */

	char*	str;

#line 382 "util/configparser.h" /* yacc.c:1909  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_UTIL_CONFIGPARSER_H_INCLUDED  */
