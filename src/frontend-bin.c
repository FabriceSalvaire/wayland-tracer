// -*- mode: C; c-basic-offset: 4 -*-

/*
 * Copyright © 2014 Boyan Ding
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include "wayland-private.h"
#include "tracer.h"
#include "frontend-bin.h"

/**************************************************************************************************/

static int
bin_init(struct tracer *tracer)
{
    return 0;
}

/**************************************************************************************************/

static int
bin_handle_data(struct tracer_connection *connection, int rlen)
{
    // this handler process all the messages

    struct tracer_instance *instance = connection->instance;
    struct wl_connection *wl_conn = connection->wl_conn;
    struct tracer_connection *peer = connection->peer;

    int len = ring_buffer_size(&wl_conn->in);
    if (len == 0)
        return 0;

    char buf[4096];
    // Fixme: len > 4096
    wl_connection_copy(wl_conn, buf, len);

    // dump bytes
    // tracer_log("%s Data dumped: %d bytes:\n",
    //            connection->side == TRACER_SERVER_SIDE ? "=>" : "<=", len);
    // for (int i = 0; i < len; i++)
    //     tracer_log_cont("%02x ", (unsigned char) buf[i]);
    // tracer_log_cont("\n");

    {
        // buffer can contain more than one message
        size_t message_count = 0;
        const char *pm = buf;
        int size;
        // header size = 8
        for (int remain = len; remain >= 8; remain -= size, pm += size) {
            message_count += 1;
            uint32_t *header = (uint32_t *)pm;
            uint32_t id = header[0];
            int opcode = header[1] & 0xffff;
            size = header[1] >> 16;
            if (len < size)
                return 0;
            tracer_log("\x1b[31m%s \x1b[32mMessage %u \x1b[35mopcode %u\x1b[0m, size %u\n",
                       connection->side == TRACER_SERVER_SIDE ? "=>" : "<=",
                       id, opcode, size);
            for (int i = 0; i < size; i++)
                tracer_log_cont("%02x ", (unsigned char) pm[i]);
            tracer_log_cont("\n");
        }
        tracer_log("      \x1b[36m%u messages\x1b[0m\n", message_count);
    }

    wl_connection_consume(wl_conn, len);
    // forward message
    wl_connection_write(peer->wl_conn, buf, len);

    // handle fd
    int fdlen = ring_buffer_size(&wl_conn->fds_in);
    ring_buffer_copy(&wl_conn->fds_in, buf, fdlen);
    fdlen /= sizeof(int32_t);
    if (fdlen != 0)
        tracer_log_cont(">>> %d Fds in control data:", fdlen);
    for (int i = 0; i < fdlen; i++) {
        int fd = ((int *) buf)[i];
        tracer_log_cont("%d ", fd);
        wl_connection_put_fd(peer->wl_conn, fd);
    }
    // Fixme: \n
    if (fdlen != 0)
      tracer_log_cont("\n");
    tracer_log_end();
    wl_conn->fds_in.tail += fdlen * sizeof(int32_t);

    return len; // no more messages to process
}

/**************************************************************************************************/

struct tracer_frontend_interface tracer_frontend_bin = {
    .init = bin_init,
    .data = bin_handle_data
};
