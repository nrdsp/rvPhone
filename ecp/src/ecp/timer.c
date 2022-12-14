#include <stdlib.h>
#include <string.h>

#include "core.h"
#include "tm.h"

int ecp_timer_create(ECPTimer *timer) {
    int rv;

    timer->head = -1;

#ifdef ECP_WITH_PTHREAD
    rv = pthread_mutex_init(&timer->mutex, NULL);
    if (rv) return ECP_ERR;
#endif

    return ECP_OK;
}

void ecp_timer_destroy(ECPTimer *timer) {
#ifdef ECP_WITH_PTHREAD
    pthread_mutex_destroy(&timer->mutex);
#endif
}

void ecp_timer_item_init(ECPTimerItem *ti, ECPConnection *conn, unsigned char mtype, ecp_timer_retry_t retry_f, unsigned short cnt, ecp_sts_t timeout) {
    ti->conn = conn;
    ti->mtype = mtype;
    ti->retry = retry_f;
    ti->cnt = cnt;
    ti->timeout = timeout;
    ti->abstime = 0;
}

int ecp_timer_push(ECPTimerItem *ti) {
    int i, is_reg, rv = ECP_OK;
    ECPConnection *conn = ti->conn;
    ECPTimer *timer = &conn->sock->timer;

    ti->abstime = ecp_tm_abstime_ms(ti->timeout);

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_lock(&timer->mutex);
    pthread_mutex_lock(&conn->mutex);
#endif
    is_reg = ecp_conn_is_reg(conn);
    if (is_reg) conn->refcount++;
#ifdef ECP_WITH_PTHREAD
    pthread_mutex_unlock(&conn->mutex);
#endif

    if (timer->head == ECP_MAX_TIMER-1) rv = ECP_ERR_MAX_TIMER;
    if (!rv && !is_reg) rv = ECP_ERR_CLOSED;

    if (!rv) {
        for (i=timer->head; i>=0; i--) {
            if (ECP_STS_LTE(ti->abstime, timer->item[i].abstime)) {
                if (i != timer->head) memmove(timer->item+i+2, timer->item+i+1, sizeof(ECPTimerItem) * (timer->head-i));
                timer->item[i+1] = *ti;
                timer->head++;
                break;
            }
        }
        if (i == -1) {
            if (timer->head != -1) memmove(timer->item+1, timer->item, sizeof(ECPTimerItem) * (timer->head+1));
            timer->item[0] = *ti;
            timer->head++;
        }
        ecp_tm_timer_set(ti->timeout);
    }

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_unlock(&timer->mutex);
#endif

    return rv;
}

void ecp_timer_pop(ECPConnection *conn, unsigned char mtype) {
    int i;
    ECPTimer *timer = &conn->sock->timer;

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_lock(&timer->mutex);
#endif

    for (i=timer->head; i>=0; i--) {
        ECPConnection *curr_conn = timer->item[i].conn;
        if ((conn == curr_conn) && (mtype == timer->item[i].mtype)) {
            if (i != timer->head) {
                memmove(timer->item+i, timer->item+i+1, sizeof(ECPTimerItem) * (timer->head-i));
                memset(timer->item+timer->head, 0, sizeof(ECPTimerItem));
            } else {
                memset(timer->item+i, 0, sizeof(ECPTimerItem));
            }
            ecp_conn_refcount_dec(conn);
            timer->head--;
            break;
        }
    }

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_unlock(&timer->mutex);
#endif
}

void ecp_timer_remove(ECPConnection *conn) {
    int i;
    ECPTimer *timer = &conn->sock->timer;

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_lock(&timer->mutex);
#endif

    for (i=timer->head; i>=0; i--) {
        ECPConnection *curr_conn = timer->item[i].conn;
        if (conn == curr_conn) {
            if (i != timer->head) {
                memmove(timer->item+i, timer->item+i+1, sizeof(ECPTimerItem) * (timer->head-i));
                memset(timer->item+timer->head, 0, sizeof(ECPTimerItem));
            } else {
                memset(timer->item+i, 0, sizeof(ECPTimerItem));
            }
            ecp_conn_refcount_dec(conn);
            timer->head--;
        }
    }

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_unlock(&timer->mutex);
#endif
}

ecp_sts_t ecp_timer_exe(ECPSocket *sock) {
    int i;
    ecp_sts_t ret = 0;
    ECPTimer *timer = &sock->timer;
    ECPTimerItem to_exec[ECP_MAX_TIMER];
    int to_exec_size = 0;
    ecp_sts_t now = ecp_tm_abstime_ms(0);

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_lock(&timer->mutex);
#endif

    for (i=timer->head; i>=0; i--) {
        ecp_sts_t abstime = timer->item[i].abstime;

        if (ECP_STS_LT(now, abstime)) {
            ret = abstime - now;
            break;
        }
        to_exec[to_exec_size] = timer->item[i];
        to_exec_size++;
    }
    if (i != timer->head) {
        memset(timer->item+i+1, 0, sizeof(ECPTimerItem) * (timer->head-i));
        timer->head = i;
    }

#ifdef ECP_WITH_PTHREAD
    pthread_mutex_unlock(&timer->mutex);
#endif

    for (i=to_exec_size-1; i>=0; i--) {
        ECPConnection *conn = to_exec[i].conn;
        unsigned char mtype = to_exec[i].mtype;
        ecp_timer_retry_t retry = to_exec[i].retry;
        int rv = ECP_OK;

        if (to_exec[i].cnt > 0) {
            to_exec[i].cnt--;
            if (retry) {
                ssize_t _rv;

                _rv = retry(conn, to_exec+i);
                if (_rv < 0) rv = _rv;
            }
            if (rv && (rv != ECP_ERR_CLOSED)) ecp_err_handle(conn, mtype, rv);
        } else {
            ecp_err_handle(conn, mtype, ECP_ERR_TIMEOUT);
        }
        ecp_conn_refcount_dec(conn);
    }

    return ret;
}

ssize_t ecp_timer_send(ECPConnection *conn, ecp_timer_retry_t send_f, unsigned char mtype, unsigned short cnt, ecp_sts_t timeout) {
    ECPTimerItem ti;

    if (cnt == 0) return ECP_ERR;

    ecp_timer_item_init(&ti, conn, mtype, send_f, cnt-1, timeout);

    return send_f(conn, &ti);
}
