CFLAGS	+= -O3 -DECP_WITH_PTHREAD=1 -DECP_WITH_HTABLE=1 -DECP_WITH_RBUF=1 -DECP_WITH_MSGQ=1 -DECP_WITH_VCONN=1 -DECP_WITH_DIRSRV -DECP_DEBUG=1
LDFLAGS	+= -lm -pthread

platform = posix