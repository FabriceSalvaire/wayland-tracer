// Not in vanilla
// Export private API
#ifndef CONNECTION_H
#define CONNECTION_H

struct wl_ring_buffer
{
    char data[4096];
    uint32_t head, tail;
};

uint32_t ring_buffer_size(struct wl_ring_buffer *b);
void ring_buffer_copy(struct wl_ring_buffer *b, void *data, size_t count);

struct wl_connection
{
    struct wl_ring_buffer in, out;
    struct wl_ring_buffer fds_in, fds_out;
    int fd;
    int want_flush;
};

int wl_connection_put_fd(struct wl_connection *connection, int32_t fd);

#endif
