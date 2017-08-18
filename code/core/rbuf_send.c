#include "core.h"

#include <string.h>

#define NACK_RATE_UNIT   10000

static ssize_t flush_send(ECPConnection *conn, ECPTimerItem *ti) {
    unsigned char payload[ECP_SIZE_PLD(0)];

    ecp_pld_set_type(payload, ECP_MTYPE_RBFLUSH);
    if (ti == NULL) {
        ECPTimerItem _ti;
        int rv = ecp_timer_item_init(&_ti, conn, ECP_MTYPE_RBACK, 3, 500);
        if (rv) return rv;

        _ti.retry = flush_send;
        rv = ecp_timer_push(&_ti);
        if (rv) return rv;
    }
    return ecp_rbuf_pld_send(conn, payload, sizeof(payload), 0);
}

static void cc_flush(ECPConnection *conn) {
    ECPRBSend *buf = conn->rbuf.send;
    ECPRBuffer *rbuf = &buf->rbuf;
    ecp_seq_t pkt_buf_cnt = rbuf->seq_max - rbuf->seq_start + 1;
    ecp_win_t pkt_cc_cnt = buf->win_size > buf->in_transit ? buf->win_size - buf->in_transit : 0;
    int pkt_to_send = pkt_buf_cnt < pkt_cc_cnt ? pkt_buf_cnt : pkt_cc_cnt;
    int i;
    
    if (pkt_to_send) {
        unsigned int idx = ecp_rbuf_msg_idx(rbuf, buf->seq_cc);
        unsigned int _idx = idx;

        for (i=0; i<pkt_to_send; i++) {
            if (!(rbuf->msg[_idx].flags & ECP_RBUF_FLAG_IN_CCONTROL)) break;
            rbuf->msg[_idx].flags &= ~ECP_RBUF_FLAG_IN_CCONTROL;
            _idx = ECP_RBUF_IDX_MASK(_idx + 1, rbuf->msg_size);
        }
        pkt_to_send = i;
        _idx = idx;

#ifdef ECP_WITH_PTHREAD
        pthread_mutex_unlock(&buf->mutex);
#endif

        for (i=0; i<pkt_to_send; i++) {
            ecp_pkt_send(conn->sock, &conn->node.addr, rbuf->msg[_idx].msg, rbuf->msg[_idx].size);
            _idx = ECP_RBUF_IDX_MASK(_idx + 1, rbuf->msg_size);
        }

#ifdef ECP_WITH_PTHREAD
        pthread_mutex_lock(&buf->mutex);
#endif

        buf->in_transit += (ecp_win_t)pkt_to_send;
        buf->cnt_cc -= (ecp_win_t)pkt_to_send; 
        buf->seq_cc += (ecp_seq_t)pkt_to_send;
    }
}

ssize_t ecp_rbuf_handle_ack(ECPConnection *conn, ecp_seq_t seq, unsigned char mtype, unsigned char *msg, ssize_t size) {
    ECPRBSend *buf;
    ssize_t rsize = sizeof(ecp_seq_t)+sizeof(ecp_ack_t);
    ecp_seq_t seq_ack = 0;
    ecp_ack_t ack_map = 0;
    int i;
    int do_flush = 0;
    int rv = ECP_OK;
    
    buf = conn->rbuf.send;
    if (buf == NULL) return ECP_ERR;
    if (size < 0) return size;
    if (size < rsize) return ECP_ERR;

    seq_ack = \
        (msg[0] << 24) | \
        (msg[1] << 16) | \
        (msg[2] << 8)  | \
        (msg[3]);
    ack_map = \
        (msg[4] << 24) | \
        (msg[5] << 16) | \
        (msg[6] << 8)  | \
        (msg[7]);

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_lock(&buf->mutex);
#endif

    ECPRBuffer *rbuf = &buf->rbuf;
    int idx = ecp_rbuf_msg_idx(rbuf, seq_ack);
    if (idx < 0) rv = idx;
    
    if (!rv) {
        seq_ack++;
        if (buf->flags & ECP_RBUF_FLAG_CCONTROL) buf->in_transit = buf->cnt_cc ? buf->seq_cc - seq_ack : rbuf->seq_max - seq_ack + 1;

        if (ack_map != ECP_RBUF_ACK_FULL) {
            int nack_first = 0;
            unsigned int msg_start;
            ecp_seq_t seq_start;
            ecp_win_t nack_cnt = 0;
            
            seq_ack -= ECP_RBUF_ACK_SIZE;

#ifdef ECP_WITH_PTHREAD
            pthread_mutex_unlock(&buf->mutex);
#endif
            
            for (i=0; i<ECP_RBUF_ACK_SIZE; i++) {
                if ((ack_map & ((ecp_ack_t)1 << (ECP_RBUF_ACK_SIZE - i - 1))) == 0) {
                    nack_cnt++;
                    if (buf->flags & ECP_RBUF_FLAG_RELIABLE) {
                        unsigned int _idx = ECP_RBUF_IDX_MASK(idx + 1 - ECP_RBUF_ACK_SIZE + i, rbuf->msg_size);
                        if ((rbuf->msg[_idx].size == 0) || (rbuf->msg[_idx].flags & ECP_RBUF_FLAG_SYS)) {
                            
                        } else {
                            ecp_pkt_send(conn->sock, &conn->node.addr, rbuf->msg[_idx].msg, rbuf->msg[_idx].size);
                        }
                        if (!nack_first) {
                            nack_first = 1;
                            seq_start = seq_ack + i;
                            msg_start = _idx;
                        }
                    }
                }
            }

#ifdef ECP_WITH_PTHREAD
            pthread_mutex_lock(&buf->mutex);
#endif

            if (buf->flags & ECP_RBUF_FLAG_CCONTROL) buf->in_transit += nack_cnt;
            buf->nack_rate = (buf->nack_rate * 7 + ((ECP_RBUF_ACK_SIZE - nack_cnt) * NACK_RATE_UNIT) / ECP_RBUF_ACK_SIZE) / 8;
            if (buf->flags & ECP_RBUF_FLAG_RELIABLE) {
                rbuf->seq_start = seq_start;
                rbuf->msg_start = msg_start;
            } else {
                rbuf->seq_start = seq_ack + ECP_RBUF_ACK_SIZE;
                rbuf->msg_start = ECP_RBUF_IDX_MASK(idx + 1, rbuf->msg_size);
            }
        } else {
            buf->nack_rate = (buf->nack_rate * 7) / 8;
            rbuf->seq_start = seq_ack;
            rbuf->msg_start = ECP_RBUF_IDX_MASK(idx + 1, rbuf->msg_size);
        }
        if (buf->flush) {
            if (ECP_RBUF_SEQ_LT(buf->seq_flush, rbuf->seq_start)) buf->flush = 0;
            if (buf->flush) {
                do_flush = 1;
            }
        }
        if (buf->cnt_cc) cc_flush(conn);
    }

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_unlock(&buf->mutex);
#endif

    if (rv) return rv;

    if (do_flush) {
        ssize_t _rv = flush_send(conn, NULL);
        if (_rv < 0) return _rv;
    }
    return rsize;
}

int ecp_rbuf_send_create(ECPConnection *conn, ECPRBSend *buf, ECPRBMessage *msg, unsigned int msg_size) {
    int rv;
    
    memset(buf, 0, sizeof(ECPRBRecv));
    rv = ecp_rbuf_init(&buf->rbuf, msg, msg_size);
    if (rv) return rv;

#ifdef ECP_WITH_PTHREAD
    rv = pthread_mutex_init(&buf->mutex, NULL);
    if (rv) return ECP_ERR;
#endif

    conn->rbuf.send = buf;
    return ECP_OK;
}

void ecp_rbuf_send_destroy(ECPConnection *conn) {
    ECPRBSend *buf = conn->rbuf.send;
    
    if (buf == NULL) return;
    
#ifdef ECP_WITH_PTHREAD
    pthread_mutex_destroy(&buf->mutex);
#endif
}

int ecp_rbuf_send_set_wsize(ECPConnection *conn, ecp_win_t size) {
    ECPRBSend *buf = conn->rbuf.send;

    if (buf == NULL) return ECP_ERR;
    
#ifdef ECP_WITH_PTHREAD
    pthread_mutex_lock(&buf->mutex);
#endif
    
    buf->win_size = size;
    if (buf->cnt_cc) cc_flush(conn);

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_unlock(&buf->mutex);
#endif

    return ECP_OK;
}

int ecp_rbuf_send_start(ECPConnection *conn) {
    ECPRBSend *buf = conn->rbuf.send;

    if (buf == NULL) return ECP_ERR;

    return ecp_rbuf_start(&buf->rbuf, conn->seq_out);
}

int ecp_rbuf_flush(ECPConnection *conn) {
    ECPRBSend *buf = conn->rbuf.send;
    unsigned char payload[ECP_SIZE_PLD(0)];
    ecp_seq_t seq;

    if (buf == NULL) return ECP_ERR;

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_lock(&conn->mutex);
#endif
    seq = conn->seq_out;
#ifdef ECP_WITH_PTHREAD
    pthread_mutex_unlock(&conn->mutex);
#endif

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_lock(&buf->mutex);
#endif

    if (buf->flush) {
        if (ECP_RBUF_SEQ_LT(buf->seq_flush, seq)) buf->seq_flush = seq;
    } else {
        buf->flush = 1;
        buf->seq_flush = seq;
    }

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_unlock(&buf->mutex);
#endif

    ssize_t rv = flush_send(conn, 0);
    if (rv < 0) return rv;

    return ECP_OK;
}

