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
#include <stat_cache.h>                         /* cf_remote_stat   */
#include <string_lib.h>                         /* SafeStringLength */

/**
 *
 * What should this function do?
 *
 * The problem with CfNetConnect, is that it will essentially be the exact same
 * function as ServerConnection.
 */
AgentConnection *CfNetConnect(char const *host, char const *port)
{
    AgentConnection *conn = NULL;
    ConnectionFlags connflags =
    {
        .protocol_version = CF_PROTOCOL_LATEST,
        .trust_server = true,
        .off_the_record = true
    };

    if (port == NULL)
    {
        port = CFENGINE_PORT_STR;
    }

    int err;
    conn = ServerConnection(host, port, 30, connflags, &err);
    if (conn == NULL)
    {
        return NULL;
    }

    return conn;
}

int CfNetDisconnect(AgentConnection *conn)
{
    DisconnectServer(conn);

    return 0;
}

Stat *CfNetStat(AgentConnection *conn, char const *path)
{
    struct stat sb;
    int ret = cf_remote_stat(conn, false, path, &sb, "file");
    if (ret == -1)
    {
        Log(LOG_LEVEL_ERR, "Some uncool error occured");
        return false;
    }

    return NULL;
}

// I suppose this function should only return whether it has received a file or
// not? How would it return a file pointer? In some cases you only want to get
// the file, and *then* edit it -- doing this separately is much better than
// creating some function that expects you to take the file pointer afterwards,
// especially because whoever receives the file will decide for themselves how
// they want to open it
bool CfNetGet(AgentConnection *conn, char const *src, char const *dst)
{
    // Here it might be an idea to stat the file first, since we need the size.
    struct stat sb;
    int ret = cf_remote_stat(conn, false, src, &sb, "file");
    if (ret == -1)
    {
        Log(LOG_LEVEL_ERR, "Some uncool error occured");
        return false;
    }

    off_t size = sb.st_size;
    char buf[CF_BUFSIZE];
    xsnprintf(buf, sizeof(buf), "GET %d %s", size, src);

    ret = SendTransaction(conn->conn_info, buf,
                          SafeStringLength(buf) + 1, CF_DONE);
    if (ret < 0)
    {
        Log(LOG_LEVEL_ERR,
            "Could not send : (");
        return false;
    }

    FILE *cuck = safe_fopen(dst, "w");
    Writer *shiet = FileWriter(cuck);

    int cunt = 0;
    while (cunt < size)
    {
        cunt += ReceiveTransaction(conn->conn_info, buf, NULL);
        ret = WriterWrite(shiet, buf);
        if (ret < 0)
        {

        }
    }

    WriterClose(shiet);
    return true;
}

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
