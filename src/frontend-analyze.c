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

#include <stdio.h>
#include <string.h>

#include "wayland-private.h"
#include "wayland-util.h"
#include "tracer.h"
#include "frontend-analyze.h"
#include "tracer-analyzer.h"

/**************************************************************************************************/

// from connection.c
static inline uint32_t
div_roundup(uint32_t n, size_t a)
{
    /* The cast to uint64_t is necessary to prevent overflow when rounding
     * values close to UINT32_MAX. After the division it is again safe to
     * cast back to uint32_t.
     */
    return (uint32_t) (((uint64_t) n + (a - 1)) / a);
}

/**************************************************************************************************/

static int
analyze_init(struct tracer *tracer)
{
    struct tracer_analyzer *analyzer;
    analyzer = tracer_analyzer_create();
    if (analyzer == NULL) {
        fprintf(stderr, "Failed to create analyzer: %m\n");
        return -1;
    }

    struct protocol_file *file;
    struct tracer_options *options = tracer->options;
    wl_list_for_each(file, &options->protocol_file_list, link) {
        if (tracer_analyzer_add_protocol(analyzer, file->loc) != 0) {
            fprintf(stderr, "failed to add file %s\n", file->loc);
            return -1;
        }
    }

    if (tracer_analyzer_finalize(analyzer) != 0)
        return -1;

    tracer->frontend_data = analyzer;

    return 0;
}

/**************************************************************************************************/

static int
analyze_protocol(struct tracer_connection *connection,
                 uint32_t size,
                 struct wl_map *objects,
                 struct tracer_interface *target,
                 uint32_t id,
                 struct tracer_message *message)
{
    uint32_t length, new_id;
    int fd;
    char *type_name;
    char buf[4096];
    uint32_t *p = (uint32_t *) buf + 2;
    struct tracer_connection *peer = connection->peer;
    struct tracer_instance *instance = connection->instance;
    struct tracer *tracer = connection->instance->tracer;

    struct tracer_analyzer * analyzer = (struct tracer_analyzer *) tracer->frontend_data;

    wl_connection_copy(connection->wl_conn, buf, size);
    if (target == NULL)
        goto finish;

    size_t count = strlen(message->signature);

    // "%s %s@%u.%s("
    tracer_log("%s \x1b[31m%s\x1b[32m@%u\x1b[34m.%s\x1b[0m(",
               connection->side == TRACER_CLIENT_SIDE ? "<-" : "->",
               target->name, id, message->name);

    const char * signature = message->signature;
    {
        tracer_log_cont(signature);
        tracer_log_cont(" -> ");
    }
    signature = message->signature;
    for (size_t i = 0; i < count; i++, signature++) {
        if (i != 0)
            tracer_log_cont(", ");

        switch (*signature) {
        case 'u': // 32-bit unsigned integer
            tracer_log_cont("%u", *p++);
            break;
        case 'i': // 32-bit signed integer
            tracer_log_cont("%i", *p++);
            break;
        case 'f': // fixed: 24.8 bit signed fixed-point numbers
            tracer_log_cont("%lf", wl_fixed_to_double(*p++));
            break;
        case 's': // string
            // prefixed with a 32-bit integer specifying its length (in bytes),
            // followed by the string contents and a NUL terminator,
            // padded to 32 bits with undefined data
            length = *p++;
            if (length == 0)
                tracer_log_cont("(null)");
            else
                tracer_log_cont("\"%s\"", (char *) p);
            p += div_roundup(length, sizeof *p);
            break;
        case 'o': // object: 32-bit object ID
            tracer_log_cont("obj %u", *p++);
            break;
        case 'n': // new_id 32-bit object ID
            // e.g. wl_display::get_registry(registry: new_id<wl_registry>)
            new_id = *p++;
            if (new_id != 0) {
                wl_map_reserve_new(objects, new_id);
                wl_map_insert_at(objects, 0, new_id, message->types[0]);
            }
            tracer_log_cont("new_id %u", new_id);
            break;
        case 'a': // A blob of arbitrary data
            // prefixed with a 32-bit integer specifying its length (in bytes),
            // then the verbatim contents of the array,
            // padded to 32 bits with undefined data
            length = *p++;
            tracer_log_cont("array: %u", length);
            p += div_roundup(length, sizeof *p);
            break;
        case 'h': // fd: 0-bit value on the primary transport,
            // but transfers a file descriptor to the other end using the ancillary data in the Unix
            // domain socket message (msg_control).
            ring_buffer_copy(&connection->wl_conn->fds_in, &fd, sizeof fd);
            connection->wl_conn->fds_in.tail += sizeof fd;
            tracer_log_cont("fd %d", fd);
            wl_connection_put_fd(peer->wl_conn, fd);
            break;
        case 'N': // new_id N = sun
            // e.g. wl_registry.bind(name: uint, id: new_id)
            // s
            length = *p++;
            if (length != 0)
                type_name = (char *) p;
            else
                type_name = NULL;
            p += div_roundup(length, sizeof *p);

            // u
            uint32_t name = *p++;

            // n
            new_id = *p++;
            if (new_id != 0) {
                wl_map_reserve_new(objects, new_id);
                struct tracer_interface **ptype = tracer_analyzer_lookup_type(analyzer, type_name);
                struct tracer_interface *type = ptype == NULL ? NULL : *ptype;
                wl_map_insert_at(objects, 0, new_id, type);
            }
            tracer_log_cont("new_id %u[%s,%u]", new_id, type_name, name);
            break;
        }
    }

    tracer_log_cont(")");
    tracer_log_end();

  finish:
    wl_connection_write(peer->wl_conn, buf, size);
    wl_connection_consume(connection->wl_conn, size);

    return 0;
}

/**************************************************************************************************/

static int
analyze_handle_data(struct tracer_connection *connection, int len)
{
    struct tracer_instance *instance = connection->instance;

    uint32_t p[2];
    wl_connection_copy(connection->wl_conn, p, sizeof p);
    uint32_t id = p[0];
    int opcode = p[1] & 0xffff;
    int size = p[1] >> 16;
    if (len < size)
        return 0;

    tracer_log("%s Message %u opcode %u, size %u\n",
               connection->side == TRACER_SERVER_SIDE ? "->" : "<-",
               id, opcode, size);
    {
      // Log message bytes
      char buf[4096];
      wl_connection_copy(connection->wl_conn, buf, len);
      for (int i = 0; i < 4*size; i++)
        tracer_log_cont("%02x ", (unsigned char) buf[i]);
      tracer_log_cont("\n");
    }

    struct tracer_message *message;
    struct tracer_interface *interface = wl_map_lookup(&instance->map, id);
    if (interface != NULL) {
        if (connection->side == TRACER_SERVER_SIDE)
            message = interface->events[opcode];
        else
            message = interface->methods[opcode];
    }
    else {
       tracer_log("\x1b[31mUnknown object %u opcode %u, size %u\x1b[0m", id, opcode, size);
       tracer_log_cont("\n\x1b[31mWarning: we can't guarantee the following result\x1b[0m");
       tracer_log_end();
    }

    analyze_protocol(connection, size, &instance->map, interface, id, message);

    if (interface != NULL && !strcmp(message->name, "destroy"))
        wl_map_remove(&instance->map, id);

    return size;
}

/**************************************************************************************************/

struct tracer_frontend_interface tracer_frontend_analyze = {
    .init = analyze_init,
    .data = analyze_handle_data
};
