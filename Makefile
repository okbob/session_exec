# contrib/session_exec/Makefile

MODULE_big = session_exec
OBJS = session_exec.o $(WIN32RES)
PGFILEDESC = "session_exec - logging facility for execution plans"

ifdef NO_PGXS
subdir = contrib/session_exec
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
else
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
endif
