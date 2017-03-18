/*
 * Copyright (C) 2016 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *
 * This file is part of DT Lua Milter.
 *
 * DT Lua Milter is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DT Lua Milter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with DT Lua Milter.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <sysexits.h>

#include <libmilter/mfapi.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifndef DEFAULT_CONFIG_SCRIPT
#define DEFAULT_CONFIG_SCRIPT "/etc/dt-lua-milter/default.lua"
#endif /* DEFAULT_CONFIG_SCRIPT */

const char *config_script = DEFAULT_CONFIG_SCRIPT;
char *config_file_buffer = NULL;
const char *reject_message = NULL;

struct dt_mlfi_private
{
	SMFICTX *ctx;
	lua_State *lua;
};

static int dt_smfi_stop(lua_State *L)
{
	smfi_stop();
	return 0;
}

static int dt_smfi_getsymval(lua_State *L)
{
	struct dt_mlfi_private *private_context;
	int argscount;
	const char *string;
	char *result;

	lua_getglobal(L, "ctx");

	private_context = (struct dt_mlfi_private *) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (private_context == NULL)
	{
		lua_pushstring(L, "\"ctx\" value is corrupted");
		lua_error(L);
		return 0;
	}

	argscount = lua_gettop(L);
	if (argscount != 1)
	{
		lua_pushstring(L, "function requires 1 argument");
		lua_error(L);
		return 0;
	}

	string = lua_tostring(L, -1);
	if (string == NULL)
	{
		lua_pushstring(L, "argument 1 is not a valid string");
		lua_error(L);
		return 0;
	}

	result = smfi_getsymval(private_context->ctx, (char*) string);
	if (result != NULL)
	{
		lua_pushstring(L, result);
	}
	else
	{
		lua_pushnil(L);
	}

	return 1;
}

static int dt_smfi_setreply(lua_State *L)
{
	struct dt_mlfi_private *private_context;
	int argscount;
	const char *str[3];
	int index;
	int result;

	lua_getglobal(L, "ctx");

	private_context = (struct dt_mlfi_private *) lua_touserdata(L, -1);
	lua_pop(L, 1);

	if (private_context == NULL)
	{
		lua_pushstring(L, "\"ctx\" value is corrupted");
		lua_error(L);
		return 0;
	}

	argscount = lua_gettop(L);
	if (argscount != 3)
	{
		lua_pushstring(L, "function requires 3 arguments");
		lua_error(L);
		return 0;
	}

	for (index = 0; index < 3; ++index)
	{
		str[index] = lua_tostring(L, index + 1);
	}

	if (str[0] == NULL)
	{
		lua_pushstring(L, "first argument cannot be nil");
		lua_error(L);
		return 0;
	}

	result = smfi_setreply(private_context->ctx, (char*) str[0], (char*) str[1], (char*) str[2]);
	lua_pushinteger(L, result);
	return 1;
}

struct dt_mlfi_private* init_private_context(SMFICTX *ctx)
{
	struct dt_mlfi_private *private_context;
	int res;

	private_context = (struct dt_mlfi_private*) malloc(sizeof(struct dt_mlfi_private));
	if (private_context == NULL)
	{
		goto init_private_context_error_1;
	}

	private_context->ctx = ctx;

	private_context->lua = luaL_newstate();
	if (private_context->lua == NULL)
	{
		goto init_private_context_error_2;
	}

	luaL_openlibs(private_context->lua);

#define dt_lua_register_constant(l, c) lua_pushinteger(l, c); lua_setglobal(l, #c)

	dt_lua_register_constant(private_context->lua, SMFIS_CONTINUE);
	dt_lua_register_constant(private_context->lua, SMFIS_REJECT);
	dt_lua_register_constant(private_context->lua, SMFIS_DISCARD);
	dt_lua_register_constant(private_context->lua, SMFIS_ACCEPT);
	dt_lua_register_constant(private_context->lua, SMFIS_TEMPFAIL);
	dt_lua_register_constant(private_context->lua, SMFIS_SKIP);
	dt_lua_register_constant(private_context->lua, SMFIS_NOREPLY);
	dt_lua_register_constant(private_context->lua, MI_SUCCESS);
	dt_lua_register_constant(private_context->lua, MI_FAILURE);

#undef dt_lua_register_constant

	lua_pushlightuserdata(private_context->lua, private_context);
	lua_setglobal(private_context->lua, "ctx");

	lua_register(private_context->lua, "smfi_stop",       &dt_smfi_stop);
	lua_register(private_context->lua, "smfi_getsymval",  &dt_smfi_getsymval);
	lua_register(private_context->lua, "smfi_setreply",   &dt_smfi_setreply);

	/*
	 * Registered functions:
	 * smfi_stop
	 * smfi_getsymval
	 * smfi_setreply
	 *
	 * Registered values:
	 * SMFIS_CONTINUE
	 * SMFIS_REJECT
	 * SMFIS_DISCARD
	 * SMFIS_ACCEPT
	 * SMFIS_TEMPFAIL
	 * SMFIS_SKIP
	 * SMFIS_NOREPLY
	 * MI_SUCCESS
	 * MI_FAILURE
	 *
	 * Additional special value:
	 * ctx
	 *
	 * Currently not registered. Only for xxfi_eom:
	 * smfi_progress
	 * smfi_quarantine
	 * smfi_addheader
	 * smfi_chgheader
	 * smfi_insheader
	 * smfi_chgfrom
	 * smfi_addrcpt
	 * smfi_addrcpt_par
	 * smfi_delrcpt
	 * smfi_replacebody
	 *
	 * Currently not registered values:
	 * SMFIS_ALL_OPTS
	 *
	 * Not registered values and functions on purpose:
	 * smfi_getpriv
	 * smfi_setpriv
	 * smfi_setmlreply - not possible to call with current interface
	 */

	res = luaL_dostring(private_context->lua, config_file_buffer);
	if (res != 0)
	{
		goto init_private_context_error_3;
	}

	return private_context;

init_private_context_error_3:
	lua_close(private_context->lua);

init_private_context_error_2:
	free(private_context);

init_private_context_error_1:
	return NULL;
}

void destroy_private_context(struct dt_mlfi_private *private_context)
{
	lua_close(private_context->lua);
	free(private_context);
}

sfsistat dt_smfi_fail(SMFICTX *ctx)
{
	if (reject_message != NULL)
	{
		smfi_setreply(ctx, "550", NULL, (char*) reject_message);
		return SMFIS_REJECT;
	}
	else
	{
		return SMFIS_TEMPFAIL;
	}
}

sfsistat dt_mlfi_connect(
	SMFICTX *ctx,
	char *hostname,
	_SOCK_ADDR *hostaddr)
{
	struct dt_mlfi_private *private_context;
	int lua_result;
	sfsistat result;

	private_context = init_private_context(ctx);
	if (private_context == NULL)
	{
		return dt_smfi_fail(ctx);
	}

	/* save the private data */
	smfi_setpriv(ctx, private_context);

	lua_settop(private_context->lua, 0);
	lua_getglobal(private_context->lua, "mlfi_connect");
	if (lua_isfunction(private_context->lua, -1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	lua_pushstring(private_context->lua, hostname);
	lua_result = lua_pcall(private_context->lua, 1, 1, 0);
	if (lua_result != 0)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	if (lua_isnumber(private_context->lua, 1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	result = lua_tointeger(private_context->lua, 1);
	if (result != SMFIS_CONTINUE)
	{
		destroy_private_context(private_context);
	}

	return result;
}

sfsistat dt_mlfi_helo(
	SMFICTX *ctx,
	char *helohost)
{
	struct dt_mlfi_private *private_context;
	int lua_result;
	sfsistat result;

	private_context = (struct dt_mlfi_private *) smfi_getpriv(ctx);

	lua_settop(private_context->lua, 0);
	lua_getglobal(private_context->lua, "mlfi_helo");
	if (lua_isfunction(private_context->lua, -1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	lua_pushstring(private_context->lua, helohost);
	lua_result = lua_pcall(private_context->lua, 1, 1, 0);
	if (lua_result != 0)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	if (lua_isnumber(private_context->lua, 1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	result = lua_tointeger(private_context->lua, 1);
	if (result != SMFIS_CONTINUE)
	{
		destroy_private_context(private_context);
	}

	return result;
}

sfsistat dt_mlfi_envfrom(
	SMFICTX *ctx,
	char **argv)
{
	struct dt_mlfi_private *private_context;
	int lua_result;
	sfsistat result;

	private_context = (struct dt_mlfi_private *) smfi_getpriv(ctx);

	lua_settop(private_context->lua, 0);
	lua_getglobal(private_context->lua, "mlfi_envfrom");
	if (lua_isfunction(private_context->lua, -1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	lua_result = 0;

	while (*argv != NULL)
	{
		lua_pushstring(private_context->lua, *argv);
		++argv;
		++lua_result;
	}

	lua_result = lua_pcall(private_context->lua, lua_result, 1, 0);
	if (lua_result != 0)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	if (lua_isnumber(private_context->lua, 1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	result = lua_tointeger(private_context->lua, 1);
	if (result != SMFIS_CONTINUE)
	{
		destroy_private_context(private_context);
	}

	return result;
}

sfsistat dt_mlfi_envrcpt(
	SMFICTX *ctx,
	char **argv)
{
	struct dt_mlfi_private *private_context;
	int lua_result;
	sfsistat result;

	private_context = (struct dt_mlfi_private *) smfi_getpriv(ctx);

	lua_settop(private_context->lua, 0);
	lua_getglobal(private_context->lua, "mlfi_envrcpt");
	if (lua_isfunction(private_context->lua, -1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	lua_result = 0;

	while (*argv != NULL)
	{
		lua_pushstring(private_context->lua, *argv);
		++argv;
		++lua_result;
	}

	lua_result = lua_pcall(private_context->lua, lua_result, 1, 0);
	if (lua_result != 0)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	if (lua_isnumber(private_context->lua, 1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	result = lua_tointeger(private_context->lua, 1);
	if (result != SMFIS_CONTINUE)
	{
		destroy_private_context(private_context);
	}

	return result;
}

sfsistat dt_mlfi_header(
	SMFICTX *ctx,
	char *headerf,
	char *headerv)
{
	struct dt_mlfi_private *private_context;
	int lua_result;
	sfsistat result;

	private_context = (struct dt_mlfi_private *) smfi_getpriv(ctx);

	lua_settop(private_context->lua, 0);
	lua_getglobal(private_context->lua, "mlfi_header");
	if (lua_isfunction(private_context->lua, -1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	lua_pushstring(private_context->lua, headerf);
	lua_pushstring(private_context->lua, headerv);
	lua_result = lua_pcall(private_context->lua, 2, 1, 0);
	if (lua_result != 0)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	if (lua_isnumber(private_context->lua, 1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	result = lua_tointeger(private_context->lua, 1);
	if (result != SMFIS_CONTINUE)
	{
		destroy_private_context(private_context);
	}

	return result;
}

sfsistat dt_mlfi_eoh(SMFICTX *ctx)
{
	struct dt_mlfi_private *private_context;
	int lua_result;
	sfsistat result;

	private_context = (struct dt_mlfi_private *) smfi_getpriv(ctx);

	lua_settop(private_context->lua, 0);
	lua_getglobal(private_context->lua, "mlfi_eoh");
	if (lua_isfunction(private_context->lua, -1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	lua_result = lua_pcall(private_context->lua, 0, 1, 0);
	if (lua_result != 0)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	if (lua_isnumber(private_context->lua, 1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	result = lua_tointeger(private_context->lua, 1);
	if (result != SMFIS_CONTINUE)
	{
		destroy_private_context(private_context);
	}

	return result;
}

sfsistat dt_mlfi_body(
	SMFICTX *ctx,
	unsigned char *bodyp,
	size_t bodylen)
{
	struct dt_mlfi_private *private_context;
	int lua_result;
	sfsistat result;

	private_context = (struct dt_mlfi_private *) smfi_getpriv(ctx);

	lua_settop(private_context->lua, 0);
	lua_getglobal(private_context->lua, "mlfi_body");
	if (lua_isfunction(private_context->lua, -1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	lua_pushlstring(private_context->lua, bodyp, bodylen);
	lua_pushinteger(private_context->lua, bodylen);
	lua_result = lua_pcall(private_context->lua, 2, 1, 0);
	if (lua_result != 0)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	if (lua_isnumber(private_context->lua, 1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	result = lua_tointeger(private_context->lua, 1);
	if (result != SMFIS_CONTINUE)
	{
		destroy_private_context(private_context);
	}

	return result;
}

sfsistat dt_mlfi_eom(SMFICTX *ctx)
{
	struct dt_mlfi_private *private_context;
	int lua_result;
	sfsistat result;

	private_context = (struct dt_mlfi_private *) smfi_getpriv(ctx);

	lua_settop(private_context->lua, 0);
	lua_getglobal(private_context->lua, "mlfi_eom");
	if (lua_isfunction(private_context->lua, -1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	lua_result = lua_pcall(private_context->lua, 0, 1, 0);
	if (lua_result != 0)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	if (lua_isnumber(private_context->lua, 1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	result = lua_tointeger(private_context->lua, 1);
	if (result != SMFIS_CONTINUE)
	{
		destroy_private_context(private_context);
	}

	return result;
}

sfsistat dt_mlfi_abort(SMFICTX *ctx)
{
	struct dt_mlfi_private *private_context;
	int lua_result;
	sfsistat result;

	private_context = (struct dt_mlfi_private *) smfi_getpriv(ctx);

	lua_settop(private_context->lua, 0);
	lua_getglobal(private_context->lua, "mlfi_abort");
	if (lua_isfunction(private_context->lua, -1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	lua_result = lua_pcall(private_context->lua, 0, 1, 0);
	if (lua_result != 0)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	if (lua_isnumber(private_context->lua, 1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	result = lua_tointeger(private_context->lua, 1);
	if (result != SMFIS_CONTINUE)
	{
		destroy_private_context(private_context);
	}

	return result;
}

sfsistat dt_mlfi_close(SMFICTX *ctx)
{
	struct dt_mlfi_private *private_context;
	int lua_result;
	sfsistat result = SMFIS_CONTINUE;

	private_context = (struct dt_mlfi_private *) smfi_getpriv(ctx);

	if (private_context != NULL)
	{
		lua_settop(private_context->lua, 0);
		lua_getglobal(private_context->lua, "mlfi_close");
		if (lua_isfunction(private_context->lua, -1) != 1)
		{
			destroy_private_context(private_context);
			return dt_smfi_fail(ctx);
		}

		lua_result = lua_pcall(private_context->lua, 0, 1, 0);
		if (lua_result != 0)
		{
			destroy_private_context(private_context);
			return dt_smfi_fail(ctx);
		}

		if (lua_isnumber(private_context->lua, 1) != 1)
		{
			destroy_private_context(private_context);
			return dt_smfi_fail(ctx);
		}

		result = lua_tointeger(private_context->lua, 1);
		destroy_private_context(private_context);
		smfi_setpriv(ctx, NULL);
	}

	return SMFIS_CONTINUE;
}

sfsistat dt_mlfi_unknown(
	SMFICTX *ctx,
	const char *cmd)
{
	struct dt_mlfi_private *private_context;
	int lua_result;
	sfsistat result;

	private_context = (struct dt_mlfi_private *) smfi_getpriv(ctx);

	lua_settop(private_context->lua, 0);
	lua_getglobal(private_context->lua, "mlfi_unknown");
	if (lua_isfunction(private_context->lua, -1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	lua_result = lua_pcall(private_context->lua, 0, 1, 0);
	if (lua_result != 0)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	if (lua_isnumber(private_context->lua, 1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	result = lua_tointeger(private_context->lua, 1);
	if (result != SMFIS_CONTINUE)
	{
		destroy_private_context(private_context);
	}

	return result;
}

sfsistat dt_mlfi_data(SMFICTX *ctx)
{
	struct dt_mlfi_private *private_context;
	int lua_result;
	sfsistat result;

	private_context = (struct dt_mlfi_private *) smfi_getpriv(ctx);

	lua_settop(private_context->lua, 0);
	lua_getglobal(private_context->lua, "mlfi_data");
	if (lua_isfunction(private_context->lua, -1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	lua_result = lua_pcall(private_context->lua, 0, 1, 0);
	if (lua_result != 0)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	if (lua_isnumber(private_context->lua, 1) != 1)
	{
		destroy_private_context(private_context);
		return dt_smfi_fail(ctx);
	}

	result = lua_tointeger(private_context->lua, 1);
	if (result != SMFIS_CONTINUE)
	{
		destroy_private_context(private_context);
	}

	return result;
}

sfsistat dt_mlfi_negotiate(
	SMFICTX *ctx,
	unsigned long f0,
	unsigned long f1,
	unsigned long f2,
	unsigned long f3,
	unsigned long *pf0,
	unsigned long *pf1,
	unsigned long *pf2,
	unsigned long *pf3)
{
	return SMFIS_ALL_OPTS;
}

struct smfiDesc smfilter =
{
	"DTFilter",	/* filter name */
	SMFI_VERSION,	/* version code -- do not change */
	SMFIF_NONE, /* flags */
	dt_mlfi_connect,	/* connection info filter */
	dt_mlfi_helo,	/* SMTP HELO command filter */
	dt_mlfi_envfrom,	/* envelope sender filter */
	dt_mlfi_envrcpt,	/* envelope recipient filter */
	dt_mlfi_header,	/* header filter */
	dt_mlfi_eoh,	/* end of header */
	dt_mlfi_body,	/* body block filter */
	dt_mlfi_eom,	/* end of message */
	dt_mlfi_abort,	/* message aborted */
	dt_mlfi_close,	/* connection cleanup */
	dt_mlfi_unknown,	/* unknown SMTP commands */
	dt_mlfi_data,	/* DATA command */
	dt_mlfi_negotiate	/* Once, at the start of each SMTP connection */
};

static int safe_str_to_int(const char *str, int *ok)
{
	int plus = 1;
	int result = 0;
	int result_ok = 1;

	if (*str == '-')
	{
		plus = 0;
		++str;
	}

	if (!*str)
	{
		result_ok = 0;
	}

	while (*str)
	{
		if (isdigit(*str))
		{
			result = result * 10 + (*str - '0');
		}
		else
		{
			result_ok = 0;
		}

		++str;
	}

	if (ok != NULL)
	{
		*ok = result_ok;
	}

	return (plus ? result : (-result));
}

static int file_exists(const char *filename)
{
	struct stat statbuf;

	return ((stat(filename, &statbuf) == 0) ? 1 : 0);
}

static char* read_full_file(const char *filename)
{
	FILE *file = NULL;
	char *buffer = NULL;
	long file_size = 0;

	file = fopen(filename, "rt");
	if (file == NULL)
	{
		goto read_full_file_error_1;
	}

	fseek(file, 0, SEEK_END);
	long fsize = ftell(file);
	fseek(file, 0, SEEK_SET);  //same as rewind(f);

	buffer = malloc(file_size + 1);
	if (buffer == NULL)
	{
		goto read_full_file_error_2;
	}

	fread(buffer, file_size, 1, file);
	buffer[file_size] = 0;

read_full_file_error_2:
	fclose(file);

read_full_file_error_1:
	return buffer;
}

static void usage(const char *app_name)
{
	fprintf(stderr, "Usage: %s -p socket-addr [-t timeout] [-c config] [-r reject_message]\n"
		"\t\n"
		"\tif reject message is set, then in case of issues REJECT code will be used instead of TEMPFAIL code\n"
		, app_name);
}

int main(int argc, char **argv)
{
	int setconn = 0;
	int c;
	int timeout = 0;
	int timeout_ok = 0;
	int result = 0;
	const char *args = "p:t:l:c:r:h";
	extern char *optarg;

	/* Process command line options */
	while ((c = getopt(argc, argv, args)) != -1)
	{
		switch (c)
		{
		case 'p':
			if ((optarg == NULL) || (*optarg == '\0'))
			{
				(void) fprintf(stderr, "Illegal conn: %s\n", optarg);
				exit(EX_USAGE);
			}

			if (smfi_setconn(optarg) == MI_FAILURE)
			{
				(void) fprintf(stderr, "smfi_setconn failed\n");
				exit(EX_SOFTWARE);
			}

			if (strncasecmp(optarg, "unix:", 5) == 0)
			{
				unlink(optarg + 5);
			}
			else if (strncasecmp(optarg, "local:", 6) == 0)
			{
				unlink(optarg + 6);
			}

			setconn = 1;
			break;

		case 't':
			if ((optarg == NULL) || (*optarg == '\0'))
			{
				(void) fprintf(stderr, "Illegal timeout: %s\n", optarg);
				exit(EX_USAGE);
			}

			timeout = safe_str_to_int(optarg, &timeout_ok);
			if (!timeout_ok)
			{
				(void) fprintf(stderr, "Illegal timeout: %s\n", optarg);
				exit(EX_USAGE);
			}

			if (smfi_settimeout(timeout) == MI_FAILURE)
			{
				(void) fprintf(stderr, "smfi_settimeout failed\n");
				exit(EX_SOFTWARE);
			}
			break;

		case 'c':
			if ((optarg == NULL) || (*optarg == '\0'))
			{
				(void) fprintf(stderr, "Config doesn't exist: %s\n", optarg);
				exit(EX_USAGE);
			}

			if (!file_exists(optarg))
			{
				(void) fprintf(stderr, "Config doesn't exist: %s\n", optarg);
				exit(EX_USAGE);
			}

			config_script = optarg;
			break;

		case 'r':
			if ((optarg == NULL) || (*optarg == '\0'))
			{
				(void) fprintf(stderr, "Illegal reject message: %s\n", optarg);
				exit(EX_USAGE);
			}

			reject_message = optarg;
			break;

		case 'h':
		default:
			usage(argv[0]);
			exit(EX_USAGE);
		}
	}

	if (!setconn)
	{
		fprintf(stderr, "%s: Missing required -p argument\n", argv[0]);
		usage(argv[0]);
		exit(EX_USAGE);
	}

	config_file_buffer = read_full_file(config_script);
	if (config_file_buffer == NULL)
	{
		(void) fprintf(stderr, "Failed to read config file: %s\n", config_script);
		exit(EX_SOFTWARE);
	}

	if (smfi_register(smfilter) == MI_FAILURE)
	{
		fprintf(stderr, "smfi_register failed\n");
		exit(EX_UNAVAILABLE);
	}

	result = smfi_main();

	free(config_file_buffer);

	return result;
}
