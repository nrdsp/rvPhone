#include "core.h"

#include <string.h>

#define ACK_RATE            8
#define ACK_MASK_FIRST      ((ecp_ack_t)1 << (ECP_RBUF_ACK_SIZE - 1))

static ssize_t msg_store(ECPConnection *conn, ecp_seq_t seq, unsigned char *msg, size_t msg_size) {
    ECPRBRecv *buf = conn->rbuf.recv;
    unsigned char flags = ECP_RBUF_FLAG_IN_RBUF;
    unsigned char mtype = msg[0] & ECP_MTYPE_MASK;
        
    if (mtype < ECP_MAX_MTYPE_SYS) flags |= ECP_RBUF_FLAG_SYS;
        
#ifdef ECP_WITH_MSGQ
    if (buf->flags & ECP_RBUF_FLAG_MSGQ) {
        int rv = ECP_OK;

        pthread_mutex_lock(&buf->msgq.mutex);
        ecp_seq_t seq_offset = seq - buf->msgq.seq_start;
        if (seq_offset >= buf->rbuf.msg_size) rv = ECP_ERR_RBUF_FULL;
        pthread_mutex_unlock(&buf->msgq.mutex);
        
        if (rv) return rv;
    }
#endif

    ssize_t rv = ecp_rbuf_msg_store(&buf->rbuf, seq, -1, msg, msg_size, ECP_RBUF_FLAG_IN_RBUF | ECP_RBUF_FLAG_IN_MSGQ, flags);
    if (rv < 0) return rv;

    if (ECP_RBUF_SEQ_LT(buf->rbuf.seq_max, seq)) buf->rbuf.seq_max = seq;
    if (flags & ECP_RBUF_FLAG_SYS) ecp_msg_handle(conn, seq, msg, msg_size);

    return rv;
}

static void msg_flush(ECPConnection *conn) {
    ECPRBRecv *buf = conn->rbuf.recv;

#ifdef ECP_WITH_MSGQ
    if (buf->flags & ECP_RBUF_FLAG_MSGQ) pthread_mutex_lock(&buf->msgq.mutex);
#endif

    ecp_seq_t msg_cnt = buf->rbuf.seq_max - buf->rbuf.seq_start + 1;
    ecp_seq_t i = 0;
    unsigned int idx = buf->rbuf.msg_start;
    
    for (i=0; i<msg_cnt; i++) {
        if ((buf->flags & ECP_RBUF_FLAG_RELIABLE) && !(buf->rbuf.msg[idx].flags & ECP_RBUF_FLAG_IN_RBUF)) break;
        if (buf->deliver_delay && (msg_cnt - i < buf->deliver_delay)) break;
        if (buf->rbuf.msg[idx].flags & ECP_RBUF_FLAG_IN_RBUF) {
            int rv = 0;
            ecp_seq_t seq = buf->rbuf.seq_start + i;
            if (buf->rbuf.msg[idx].flags & ECP_RBUF_FLAG_SYS) {
                buf->rbuf.msg[idx].flags &= ~ECP_RBUF_FLAG_SYS;
            } else if (buf->flags & ECP_RBUF_FLAG_MSGQ) {
#ifdef ECP_WITH_MSGQ
                unsigned char mtype = buf->rbuf.msg[idx].msg[0];
                int rv = ecp_conn_msgq_push(conn, seq, mtype);
                if (rv) break;
                buf->rbuf.msg[idx].flags |= ECP_RBUF_FLAG_IN_MSGQ;
#endif
            } else {
                ecp_msg_handle(conn, seq, buf->rbuf.msg[idx].msg, buf->rbuf.msg[idx].size);
            }
            buf->rbuf.msg[idx].flags &= ~ECP_RBUF_FLAG_IN_RBUF;
        }
        idx = ECP_RBUF_IDX_MASK(idx + 1, buf->rbuf.msg_size);
    }
    buf->rbuf.seq_start += i;
    buf->rbuf.msg_start = idx;

#ifdef ECP_WITH_MSGQ
    if (buf->flags & ECP_RBUF_FLAG_MSGQ) pthread_mutex_unlock(&buf->msgq.mutex);
#endif

}

static int ack_send(ECPConnection *conn) {
    ECPRBRecv *buf = conn->rbuf.recv;
    unsigned char payload[ECP_SIZE_PLD(sizeof(ecp_seq_t) + sizeof(ecp_ack_t))];
    unsigned char *buf_ = ecp_pld_get_buf(payload);
    ssize_t rv;

    ecp_pld_set_type(payload, ECP_MTYPE_RBACK);
    buf_[0] = (buf->seq_ack & 0xFF000000) >> 24;
    buf_[1] = (buf->seq_ack & 0x00FF0000) >> 16;
    buf_[2] = (buf->seq_ack & 0x0000FF00) >> 8;
    buf_[3] = (buf->seq_ack & 0x000000FF);
    buf_[4] = (buf->ack_map & 0xFF000000) >> 24;
    buf_[5] = (buf->ack_map & 0x00FF0000) >> 16;
    buf_[6] = (buf->ack_map & 0x0000FF00) >> 8;
    buf_[7] = (buf->ack_map & 0x000000FF);

    rv = ecp_rbuf_pld_send(conn, payload, sizeof(payload), 0);
    if (rv < 0) return rv;

    buf->ack_pkt = 0;
    return ECP_OK;
}

static int ack_shift(ECPRBRecv *buf) {
    int do_ack = 0;
    int idx;
    int i;
    
    if ((buf->flags & ECP_RBUF_FLAG_RELIABLE) && ((buf->ack_map & ACK_MASK_FIRST) == 0)) return 0;

    idx = ecp_rbuf_msg_idx(&buf->rbuf, buf->seq_ack);
    if (idx < 0) return idx;

    while (ECP_RBUF_SEQ_LT(buf->seq_ack, buf->rbuf.seq_max)) {
        idx = ECP_RBUF_IDX_MASK(idx + 1, buf->rbuf.msg_size);
        buf->seq_ack++;

        if ((buf->rbuf.msg[idx].flags & ECP_RBUF_FLAG_IN_RBUF) && (buf->ack_map == ECP_RBUF_ACK_FULL)) continue;
        
        buf->ack_map = buf->ack_map << 1;
        if (buf->rbuf.msg[idx].flags & ECP_RBUF_FLAG_IN_RBUF) {
            buf->ack_map |= 1;
        } else if (!do_ack && ECP_RBUF_SEQ_LTE(buf->seq_ack, buf->rbuf.seq_max - 2 * buf->hole_max)) {
            do_ack = 1;
        }

        if ((buf->ack_map & ACK_MASK_FIRST) == 0) break;
    }
    
    if (!do_ack && (buf->seq_ack == buf->rbuf.seq_max) && ((buf->ack_map & buf->hole_mask_full) != buf->hole_mask_full)) {
        ecp_ack_t hole_mask = buf->ack_map;

        for (i=0; i<buf->hole_max-1; i++) {
            hole_mask = hole_mask >> 1;
            if ((hole_mask & buf->hole_mask_empty) == 0) {
                do_ack = 1;
                break;
            }
        }
    }
    
    return do_ack;
}

ssize_t ecp_rbuf_handle_flush(ECPConnection *conn, ecp_seq_t seq, unsigned char mtype, unsigned char *msg, ssize_t size) {
    if (size < 0) return size;
    
    ECPRBRecv *buf = conn->rbuf.recv;
    if (buf == NULL) return ECP_ERR;

    buf->flush = 1;
    return 0;
}

int ecp_rbuf_recv_create(ECPConnection *conn, ECPRBRecv *buf, ECPRBMessage *msg, unsigned int msg_size) {
    int rv;

    memset(buf, 0, sizeof(ECPRBRecv));
    rv = ecp_rbuf_init(&buf->rbuf, msg, msg_size);
    if (rv) return rv;
    
    buf->ack_map = ECP_RBUF_ACK_FULL;
    buf->ack_rate = ACK_RATE;

#ifdef ECP_WITH_MSGQ
    rv = ecp_conn_msgq_create(conn);
    if (rv) return rv;
#endif

    conn->rbuf.recv = buf;
    return ECP_OK;
}

void ecp_rbuf_recv_destroy(ECPConnection *conn) {
#ifdef ECP_WITH_MSGQ
    ecp_conn_msgq_destroy(conn);
#endif    
}

int ecp_rbuf_recv_set_hole(ECPConnection *conn, unsigned short hole_max) {
    ECPRBRecv *buf = conn->rbuf.recv;

    buf->hole_max = hole_max;
    buf->hole_mask_full = ~(~((ecp_ack_t)1) << (hole_max * 2));
    buf->hole_mask_empty = ~(~((ecp_ack_t)1) << (hole_max + 1));
    
    return ECP_OK;
}

int ecp_rbuf_recv_set_delay(ECPConnection *conn, unsigned short delay) {
    ECPRBRecv *buf = conn->rbuf.recv;

    buf->deliver_delay = delay;
    if (buf->hole_max < delay - 1) {
        ecp_rbuf_recv_set_hole(conn, delay - 1);
    }
    
    return ECP_OK;
}

int ecp_rbuf_recv_start(ECPConnection *conn, ecp_seq_t seq) {
    int rv;
    ECPRBRecv *buf = conn->rbuf.recv;

    if (buf == NULL) return ECP_ERR;
    
    buf->seq_ack = seq;
    rv = ecp_rbuf_start(&buf->rbuf, seq);
    if (rv) return rv;

#ifdef ECP_WITH_MSGQ
    if (buf->flags & ECP_RBUF_FLAG_MSGQ) {
        rv = ecp_conn_msgq_start(conn, seq);
        if (rv) return rv;
    }
#endif
    
    return ECP_OK;
}

ssize_t ecp_rbuf_recv_store(ECPConnection *conn, ecp_seq_t seq, unsigned char *msg, size_t msg_size) {
    ECPRBRecv *buf = conn->rbuf.recv;
    ecp_seq_t ack_pkt = 0;
    ssize_t rv;
    int do_ack = 0; 
    
    if (buf == NULL) return ECP_ERR;
    if (msg_size < 1) return ECP_ERR_MIN_MSG;
    
    // XXX mtype == RBACK | RBFLUSH handle imediately
    if (ECP_RBUF_SEQ_LT(buf->rbuf.seq_max, seq)) ack_pkt = seq - buf->rbuf.seq_max;
    if (ECP_RBUF_SEQ_LTE(seq, buf->seq_ack)) {
        ecp_seq_t seq_offset = buf->seq_ack - seq;
        if (seq_offset < ECP_RBUF_ACK_SIZE) {
            ecp_ack_t ack_mask = ((ecp_ack_t)1 << seq_offset);

            if (ack_mask & buf->ack_map) return ECP_ERR_RBUF_DUP;

            buf->ack_map |= ack_mask;
            do_ack = ack_shift(buf);

            rv = msg_store(conn, seq, msg, msg_size);
            if (rv < 0) return rv;
        } else {
            return ECP_ERR_RBUF_DUP;
        }
    } else {
        if ((buf->ack_map == ECP_RBUF_ACK_FULL) && (seq == (ecp_seq_t)(buf->seq_ack + 1))) {
            if ((buf->flags & ECP_RBUF_FLAG_MSGQ) || buf->deliver_delay) {
                rv = msg_store(conn, seq, msg, msg_size);
                if (rv < 0) return rv;
            } else {
                ecp_msg_handle(conn, seq, msg, msg_size);
                rv = msg_size;
                buf->rbuf.seq_max++;
                buf->rbuf.seq_start++;
                buf->rbuf.msg_start = ECP_RBUF_IDX_MASK(buf->rbuf.msg_start + 1, buf->rbuf.msg_size);
            }
            buf->seq_ack++;
        } else {
            rv = msg_store(conn, seq, msg, msg_size);
            if (rv < 0) return rv;

            do_ack = ack_shift(buf);
        }
    }
    msg_flush(conn);
    if (buf->flush) {
        buf->flush = 0;
        do_ack = 1;
    }
    if (ack_pkt && !do_ack) {
        buf->ack_pkt += ack_pkt;
        // should send acks more aggresively when reliable and ack_map is not full (rate ~ PPS * RTT)
        if (buf->ack_pkt > buf->ack_rate) do_ack = 1;
    }
    if (do_ack) {
        int _rv = ack_send(conn);
        if (_rv) return _rv;
    }
    return rv;
}

