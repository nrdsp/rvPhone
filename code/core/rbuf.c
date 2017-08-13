#include "core.h"

#include <string.h>

int ecp_rbuf_init(ECPRBuffer *rbuf, ECPRBMessage *msg, unsigned int msg_size) {
    rbuf->msg = msg;
    if (msg_size) {
        if (msg == NULL) return ECP_ERR;
        rbuf->msg_size = msg_size;
        memset(rbuf->msg, 0, sizeof(ECPRBMessage) * msg_size);
    } else {
        rbuf->msg_size = ECP_RBUF_SEQ_HALF;
    }

    return ECP_OK;
}

int ecp_rbuf_start(ECPRBuffer *rbuf, ecp_seq_t seq) {
    rbuf->seq_max = seq;
    rbuf->seq_start = seq + 1;

    return ECP_OK;
}

int ecp_rbuf_msg_idx(ECPRBuffer *rbuf, ecp_seq_t seq) {
    ecp_seq_t seq_offset = seq - rbuf->seq_start;

    // This also checks for seq_start <= seq if seq type range >> rbuf->msg_size
    if (seq_offset < rbuf->msg_size) return ECP_RBUF_IDX_MASK(rbuf->msg_start + seq_offset, rbuf->msg_size);
    return ECP_ERR_RBUF_IDX;
}

ssize_t ecp_rbuf_msg_store(ECPRBuffer *rbuf, ecp_seq_t seq, int idx, unsigned char *msg, size_t msg_size, unsigned char test_flags, unsigned char set_flags) {
    idx = idx < 0 ? ecp_rbuf_msg_idx(rbuf, seq) : idx;
    if (idx < 0) return idx;

    if (rbuf->msg == NULL) return 0;
    if (test_flags && (test_flags & rbuf->msg[idx].flags)) return ECP_ERR_RBUF_FLAG;
    
    if (!(set_flags & ECP_RBUF_FLAG_DELIVERED)) {
        memcpy(rbuf->msg[idx].msg, msg, msg_size);
        rbuf->msg[idx].size = msg_size;
    }
    rbuf->msg[idx].flags = set_flags;

    return msg_size;
}

ssize_t ecp_conn_rbuf_pkt_send(ECPConnection *conn, ECPNetAddr *addr, unsigned char *packet, size_t pkt_size, ecp_seq_t seq, int idx) {
    int do_send = 1;
    ECPRBSend *buf = conn->rbuf.send;
    
    ssize_t rv = ecp_rbuf_msg_store(&buf->rbuf, seq, idx, packet, pkt_size, 0, 0);
    if (rv < 0) return rv;

    if (buf->win_size) {
#ifdef ECP_WITH_PTHREAD
        pthread_mutex_lock(&buf->mutex);
#endif

        if (buf->cc_wait || (buf->in_transit >= buf->win_size)) {
            if (!buf->cc_wait) buf->seq_cc = seq;
            buf->cc_wait++;
            buf->rbuf.msg[idx].flags |= ECP_RBUF_FLAG_CCWAIT;
            do_send = 0;
        } else {
            buf->in_transit++;
        }
        
#ifdef ECP_WITH_PTHREAD
        pthread_mutex_unlock(&buf->mutex);
#endif
    }

    if (do_send) rv = ecp_pkt_send(conn->sock, addr, packet, pkt_size);
    return rv;
}

int ecp_conn_rbuf_create(ECPConnection *conn, ECPRBSend *buf_s, ECPRBMessage *msg_s, unsigned int msg_s_size, ECPRBRecv *buf_r, ECPRBMessage *msg_r, unsigned int msg_r_size) {
    int rv;
    
    rv = ecp_conn_rbuf_send_create(conn, buf_s, msg_s, msg_s_size);
    if (rv) return rv;
    
    rv = ecp_conn_rbuf_recv_create(conn, buf_r, msg_r, msg_r_size);
    if (rv) {
        ecp_conn_rbuf_send_destroy(conn);
        return rv;
    }
    
    return ECP_OK;
}

void ecp_conn_rbuf_destroy(ECPConnection *conn) {
    ecp_conn_rbuf_send_destroy(conn);
    ecp_conn_rbuf_recv_destroy(conn);
}

int ecp_conn_rbuf_start(ECPConnection *conn, ecp_seq_t seq) {
    int rv = ecp_conn_rbuf_send_start(conn);
    if (rv) return rv;

    if (!conn->out) {
        rv = ecp_conn_rbuf_recv_start(conn, seq);
        if (rv) return rv;
    }
    
    return ECP_OK;
}

