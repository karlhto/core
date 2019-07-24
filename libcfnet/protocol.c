/*
   Copyright 2019 Northern.tech AS

   This file is part of CFEngine 3 - written and maintained by Northern.tech AS.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

  To the extent this program is licensed as part of the Enterprise
  versions of CFEngine, the applicable Commercial Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

#include <protocol.h>

#include <client_code.h>
#include <client_protocol.h>
#include <definitions.h>
#include <net.h>


Seq *CfNetOpenDir(AgentConnection *conn, char const *path)
{
    char buf[CF_BUFSIZE] = {0};
    snprintf(buf, CF_BUFSIZE, "OPENDIR %s", path);
    size_t tosend = strlen(buf);

    if (SendTransaction(conn->conn_info, buf, tosend, CF_DONE) == -1)
    {
        return NULL;
    }

    Seq *seq = SeqNew(0, free);

    bool more = true;
    while (more == true)
    {
        if (ReceiveTransaction(conn->conn_info, buf, (int *)&more) == -1)
        {
            break;
        }

        if (FailedProtoReply(buf))
        {
            Log(LOG_LEVEL_INFO, "Network access to '%s:%s' denied",
                conn->this_server, path);
            break;
        }

        if (BadProtoReply(buf))
        {
            Log(LOG_LEVEL_INFO, "%s", buf + strlen("BAD: "));
            break;
        }

        // FIXME Placeholder for the time being. There is probably a cleaner
        //       and safer way to do this
        for (int i = 0; buf[i] != '\0'; i += strlen(buf + i) + 1)
        {
            if (strcmp(buf + i, CFD_TERMINATOR) == 0)
            {
                more = false;
                break;
            }

            char *str = xstrdup(buf + i);
            SeqAppend(seq, str);
        }
    }

    return seq;
}
