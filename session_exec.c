/*------------------------------------------------------------------------
 *
 * session_exec.c
 *
 *
 * Copyright (c) 2015, Pavel Stehule, pavel.stehule@gmail.com
 *
 * IDENTIFICATION
 *	  contrib/session_exec/session_exec.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "miscadmin.h"
#include "fmgr.h"

#if PG_VERSION_NUM >= 90500

#include "access/parallel.h"

#endif

#include "access/xact.h"
#include "catalog/namespace.h"
#include "storage/ipc.h"
#include "utils/builtins.h"
#include "utils/guc.h"

#if PG_VERSION_NUM >= 100000

#include "utils/regproc.h"

#endif



PG_MODULE_MAGIC;

void		_PG_init(void);

char *session_login_function_name = NULL;
bool session_login_enable = true;

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

#if PG_VERSION_NUM >= 90500

	/* leave early, when current process is not typical session */
	if ((IsBackgroundWorker || InitializingParallelWorker) || !OidIsValid(MyDatabaseId))
	{
		return;
	}

#endif

	DefineCustomStringVariable("session_exec.login_name",
					 "Define function that will be executed on login",
					 "It is undefined by default",
					 &session_login_function_name,
					 NULL,
					 PGC_SUSET,
					 0, NULL, NULL, NULL);

	DefineCustomBoolVariable("session_exec.enable_login",
					 "Enable to usage login function",
					 NULL,
					 &session_login_enable,
					 true,
					 PGC_SUSET,
					 0, NULL, NULL, NULL);

	if (session_login_enable && session_login_function_name != NULL && *session_login_function_name != '\0')
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
