/*-------------------------------------------------------------------------
 *
 * session_exec.c
 *
 *
 * Copyright (c) 2008-2015, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *	  contrib/session_exec/session_exec.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "fmgr.h"
#include "access/xact.h"
#include "catalog/namespace.h"
#include "storage/ipc.h"
#include "utils/builtins.h"
#include "utils/guc.h"

PG_MODULE_MAGIC;

void		_PG_init(void);
void		_PG_fini(void);

char *session_login_function_name = NULL;

/*
 * Execute function named funcname. This function
 * has not to have any parameters. This routine will
 * be used for execution login/logout functions.
 */
static void
exec_function(char *funcname)
{
	FuncCandidateList clist;
	List *names;

	names = stringToQualifiedNameList(funcname);
	clist = FuncnameGetCandidates(names, 0, NIL, false, false, true);

	if (clist == NULL)
		elog(WARNING, "function \"%s()\" does not exist", funcname);
	else
	{
		/* execute function */
		OidFunctionCall0(clist->oid);
	}
}

void
_PG_init(void)
{
	DefineCustomStringVariable("session_exec.login_name",
	                           "Define function that will be executed on login",
	                           "It is undefined by default",
	                           &session_login_function_name,
	                           NULL,
	                           PGC_USERSET,
	                           0, NULL, NULL, NULL);

	if (session_login_function_name != NULL && *session_login_function_name != '\0')
	{
		MemoryContext	oldCtx = CurrentMemoryContext;

		PG_TRY();
		{
			SetCurrentStatementStartTimestamp();
			StartTransactionCommand();

			exec_function(session_login_function_name);

			CommitTransactionCommand();
		}
		PG_CATCH();
		{
			ErrorData	*edata;

			MemoryContextSwitchTo(oldCtx);
			edata = CopyErrorData();
			FlushErrorState();

			ereport(FATAL,
					(errcode(ERRCODE_CONNECTION_FAILURE),
					 errmsg("unhandled exception in login function \"%s\"",
						    session_login_function_name),
					 errdetail("%s", edata->message),
					 errcontext("session_exec: perform login function \"%s\"",
						    session_login_function_name),
					 errcontext("%s", edata->context)));
		}
		PG_END_TRY();
	}
}