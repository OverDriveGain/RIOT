/*
 * Copyright (C) 2015 Martine Lenders <mlenders@inf.fu-berlin.de>
 * Copyright (C) 2015 INRIA
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @{
 *
 * @file
 */

//<<<<<<< HEAD
#include <string.h>

#include "bitfield.h"
#include "net/ethernet.h"
#include "net/ipv6.h"
#include "net/gnrc.h"
#ifdef MODULE_GNRC_IPV6_NIB
#include "net/gnrc/ipv6/nib.h"
#include "net/gnrc/ipv6.h"
#endif /* MODULE_GNRC_IPV6_NIB */
#ifdef MODULE_NETSTATS_IPV6
#include "net/netstats.h"
#endif
#include "fmt.h"
#include "log.h"
#include "sched.h"

//=======MZTODO these are not conflicting
#include <errno.h>
#include "kernel_types.h"
//>>>>>>> Fix dependencies, make javascript example run
#include "net/gnrc/netif.h"

#ifdef MODULE_GNRC_IPV6_NETIF
#include "net/gnrc/ipv6/netif.h"
#endif
                default:
                    /* take from device */
                    break;
            }
            break;
#ifdef MODULE_GNRC_IPV6
        case NETOPT_IPV6_ADDR: {
                assert(opt->data_len >= sizeof(ipv6_addr_t));
                ipv6_addr_t *tgt = opt->data;

                res = 0;
                for (unsigned i = 0;
                     (res < (int)opt->data_len) && (i < GNRC_NETIF_IPV6_ADDRS_NUMOF);
                     i++) {
                    if (netif->ipv6.addrs_flags[i] != 0) {
                        memcpy(tgt, &netif->ipv6.addrs[i], sizeof(ipv6_addr_t));
                        res += sizeof(ipv6_addr_t);
                        tgt++;
                    }
                }
            }
            break;
        case NETOPT_IPV6_ADDR_FLAGS: {
                assert(opt->data_len >= sizeof(uint8_t));
                uint8_t *tgt = opt->data;

                res = 0;
                for (unsigned i = 0;
                     (res < (int)opt->data_len) && (i < GNRC_NETIF_IPV6_ADDRS_NUMOF);
                     i++) {
                    if (netif->ipv6.addrs_flags[i] != 0) {
                        *tgt = netif->ipv6.addrs_flags[i];
                        res += sizeof(uint8_t);
                        tgt++;
                    }
                }
            }
            break;
        case NETOPT_IPV6_GROUP: {
                assert(opt->data_len >= sizeof(ipv6_addr_t));
                ipv6_addr_t *tgt = opt->data;

                res = 0;
                for (unsigned i = 0;
                     (res < (int)opt->data_len) && (i < GNRC_NETIF_IPV6_GROUPS_NUMOF);
                     i++) {
                    if (!ipv6_addr_is_unspecified(&netif->ipv6.groups[i])) {
                        memcpy(tgt, &netif->ipv6.groups[i], sizeof(ipv6_addr_t));
                        res += sizeof(ipv6_addr_t);
                        tgt++;
                    }
                }
            }
            break;
        case NETOPT_IPV6_IID:
            assert(opt->data_len >= sizeof(eui64_t));
            if (gnrc_netif_ipv6_get_iid(netif, opt->data) == 0) {
                res = sizeof(eui64_t);
            }
            break;
        case NETOPT_MAX_PACKET_SIZE:
            if (opt->context == GNRC_NETTYPE_IPV6) {
                assert(opt->data_len == sizeof(uint16_t));
                *((uint16_t *)opt->data) = netif->ipv6.mtu;
                res = sizeof(uint16_t);
            }
            /* else ask device */
            break;
#if GNRC_IPV6_NIB_CONF_ROUTER
        case NETOPT_IPV6_FORWARDING:
            assert(opt->data_len == sizeof(netopt_enable_t));
            *((netopt_enable_t *)opt->data) = (gnrc_netif_is_rtr(netif)) ?
                                              NETOPT_ENABLE : NETOPT_DISABLE;
            res = sizeof(netopt_enable_t);
            break;
        case NETOPT_IPV6_SND_RTR_ADV:
            assert(opt->data_len == sizeof(netopt_enable_t));
            *((netopt_enable_t *)opt->data) = (gnrc_netif_is_rtr_adv(netif)) ?
                                              NETOPT_ENABLE : NETOPT_DISABLE;
            res = sizeof(netopt_enable_t);
            break;
#endif  /* GNRC_IPV6_NIB_CONF_ROUTER */
#endif  /* MODULE_GNRC_IPV6 */
#ifdef MODULE_GNRC_SIXLOWPAN_IPHC
        case NETOPT_6LO_IPHC:
            assert(opt->data_len == sizeof(netopt_enable_t));
            *((netopt_enable_t *)opt->data) = (netif->flags & GNRC_NETIF_FLAGS_6LO_HC) ?
                                              NETOPT_ENABLE : NETOPT_DISABLE;
            res = sizeof(netopt_enable_t);
            break;
#endif  /* MODULE_GNRC_SIXLOWPAN_IPHC */
        default:
            break;
    }
    if (res == -ENOTSUP) {
        res = netif->dev->driver->get(netif->dev, opt->opt, opt->data, opt->data_len);
    }
    gnrc_netif_release(netif);
    return res;
}

int gnrc_netif_set_from_netdev(gnrc_netif_t *netif,
                               const gnrc_netapi_opt_t *opt)
{
    int res = -ENOTSUP;

    gnrc_netif_acquire(netif);
    switch (opt->opt) {
        case NETOPT_HOP_LIMIT:
            assert(opt->data_len == sizeof(uint8_t));
            netif->cur_hl = *((uint8_t *)opt->data);
            res = sizeof(uint8_t);
            break;
#ifdef MODULE_GNRC_IPV6
        case NETOPT_IPV6_ADDR: {
                assert(opt->data_len == sizeof(ipv6_addr_t));
                /* always assume manually added */
                uint8_t flags = ((((uint8_t)opt->context & 0xff) &
                                  ~GNRC_NETIF_IPV6_ADDRS_FLAGS_STATE_MASK) |
                                 GNRC_NETIF_IPV6_ADDRS_FLAGS_STATE_VALID);
                uint8_t pfx_len = (uint8_t)(opt->context >> 8U);
                /* acquire locks a recursive mutex so we are safe calling this
                 * public function */
                res = gnrc_netif_ipv6_addr_add_internal(netif, opt->data,
                                                        pfx_len, flags);
                if (res >= 0) {
                    res = sizeof(ipv6_addr_t);
                }
            }
            break;
        case NETOPT_IPV6_ADDR_REMOVE:
            assert(opt->data_len == sizeof(ipv6_addr_t));
            /* acquire locks a recursive mutex so we are safe calling this
             * public function */
            gnrc_netif_ipv6_addr_remove_internal(netif, opt->data);
            res = sizeof(ipv6_addr_t);
            break;
        case NETOPT_IPV6_GROUP:
            assert(opt->data_len == sizeof(ipv6_addr_t));
            /* acquire locks a recursive mutex so we are safe calling this
             * public function */
            res = gnrc_netif_ipv6_group_join_internal(netif, opt->data);
            if (res >= 0) {
                res = sizeof(ipv6_addr_t);
            }
            break;
        case NETOPT_IPV6_GROUP_LEAVE:
            assert(opt->data_len == sizeof(ipv6_addr_t));
            /* acquire locks a recursive mutex so we are safe calling this
             * public function */
            gnrc_netif_ipv6_group_leave_internal(netif, opt->data);
            res = sizeof(ipv6_addr_t);
            break;
        case NETOPT_MAX_PACKET_SIZE:
            if (opt->context == GNRC_NETTYPE_IPV6) {
                assert(opt->data_len == sizeof(uint16_t));
                netif->ipv6.mtu = *((uint16_t *)opt->data);
                res = sizeof(uint16_t);
            }
            /* else set device */
            break;
#if GNRC_IPV6_NIB_CONF_ROUTER
        case NETOPT_IPV6_FORWARDING:
            assert(opt->data_len == sizeof(netopt_enable_t));
            if (*(((netopt_enable_t *)opt->data)) == NETOPT_ENABLE) {
                netif->flags |= GNRC_NETIF_FLAGS_IPV6_FORWARDING;
            }
            else {
                if (gnrc_netif_is_rtr_adv(netif)) {
                    gnrc_ipv6_nib_change_rtr_adv_iface(netif, false);
                }
                netif->flags &= ~GNRC_NETIF_FLAGS_IPV6_FORWARDING;
            }
            res = sizeof(netopt_enable_t);
            break;
        case NETOPT_IPV6_SND_RTR_ADV:
            assert(opt->data_len == sizeof(netopt_enable_t));
            gnrc_ipv6_nib_change_rtr_adv_iface(netif,
                    (*(((netopt_enable_t *)opt->data)) == NETOPT_ENABLE));
            res = sizeof(netopt_enable_t);
            break;
#endif  /* GNRC_IPV6_NIB_CONF_ROUTER */
#endif  /* MODULE_GNRC_IPV6 */
#ifdef MODULE_GNRC_SIXLOWPAN_IPHC
        case NETOPT_6LO_IPHC:
            assert(opt->data_len == sizeof(netopt_enable_t));
            if (*(((netopt_enable_t *)opt->data)) == NETOPT_ENABLE) {
                netif->flags |= GNRC_NETIF_FLAGS_6LO_HC;
            }
            else {
                netif->flags &= ~GNRC_NETIF_FLAGS_6LO_HC;
            }
            res = sizeof(netopt_enable_t);
            break;
#endif  /* MODULE_GNRC_SIXLOWPAN_IPHC */
        case NETOPT_RAWMODE:
            if (*(((netopt_enable_t *)opt->data)) == NETOPT_ENABLE) {
                netif->flags |= GNRC_NETIF_FLAGS_RAWMODE;
            }
            else {
                netif->flags &= ~GNRC_NETIF_FLAGS_RAWMODE;
            }
            /* Also propagate to the netdev device */
            netif->dev->driver->set(netif->dev, NETOPT_RAWMODE, opt->data,
                                      opt->data_len);
            res = sizeof(netopt_enable_t);
            break;
        default:
            break;
    }
    if (res == -ENOTSUP) {
        res = netif->dev->driver->set(netif->dev, opt->opt, opt->data,
                                      opt->data_len);
        if (res > 0) {
            switch (opt->opt) {
                case NETOPT_ADDRESS:
                case NETOPT_ADDRESS_LONG:
                case NETOPT_ADDR_LEN:
                case NETOPT_SRC_LEN:
                    _update_l2addr_from_dev(netif);
                    break;
                case NETOPT_STATE:
                    if (*((netopt_state_t *)opt->data) == NETOPT_STATE_RESET) {
                        _configure_netdev(netif->dev);
                    }
                    break;
                default:
                    break;
            }
        }
    }
    gnrc_netif_release(netif);
    return res;
}

gnrc_netif_t *gnrc_netif_get_by_pid(kernel_pid_t pid)
{
    gnrc_netif_t *netif = NULL;

    while ((netif = gnrc_netif_iter(netif))) {
        if (netif->pid == pid) {
            return netif;
        }
    }
    return NULL;
}

char *gnrc_netif_addr_to_str(const uint8_t *addr, size_t addr_len, char *out)
{
    char *res = out;

    assert((out != NULL) && ((addr != NULL) || (addr_len == 0U)));
    out[0] = '\0';
    for (size_t i = 0; i < addr_len; i++) {
        out += fmt_byte_hex((out), *(addr++));
        *(out++) = (i == (addr_len - 1)) ? '\0' : ':';
    }
    return res;
}

static inline int _dehex(char c, int default_)
{
    if ('0' <= c && c <= '9') {
        return c - '0';
    }
    else if ('A' <= c && c <= 'F') {
        return c - 'A' + 10;
    }
    else if ('a' <= c && c <= 'f') {
        return c - 'a' + 10;
    }
    else {
        return default_;
    }
}

size_t gnrc_netif_addr_from_str(const char *str, uint8_t *out)
{
    /* Walk over str from the end. */
    /* Take two chars a time as one hex value (%hhx). */
    /* Leading zeros can be omitted. */
    /* Every non-hexadimal character is a delimiter. */
    /* Leading, tailing and adjacent delimiters are forbidden. */
    const char *end_str = str;
    uint8_t *out_end = out;
    size_t count = 0;
    int assert_cell = 1;

    assert(out != NULL);
    if ((str == NULL) || (str[0] == '\0')) {
        return 0;
    }
    /* find end of string */
    while (end_str[1]) {
        ++end_str;
    }
    while (end_str >= str) {
        int a = 0, b = _dehex(*end_str--, -1);

        if (b < 0) {
            if (assert_cell) {
                return 0;
            }
            else {
                assert_cell = 1;
                continue;
            }
        }
        assert_cell = 0;
        if (end_str >= str) {
            a = _dehex(*end_str--, 0);
        }
        count++;
        *out_end++ = (a << 4) | b;
    }
    if (assert_cell) {
        return 0;
    }
    /* out is reversed */
    while (out < --out_end) {
        uint8_t tmp = *out_end;
        *out_end = *out;
        *out++ = tmp;
    }
    return count;
}

void gnrc_netif_acquire(gnrc_netif_t *netif)
{
    if (netif && (netif->ops)) {
        rmutex_lock(&netif->mutex);
    }
}

void gnrc_netif_release(gnrc_netif_t *netif)
{
    if (netif && (netif->ops)) {
        rmutex_unlock(&netif->mutex);
    }
}

#ifdef MODULE_GNRC_IPV6
static inline bool _addr_anycast(const gnrc_netif_t *netif, unsigned idx);
static int _addr_idx(const gnrc_netif_t *netif, const ipv6_addr_t *addr);
static int _group_idx(const gnrc_netif_t *netif, const ipv6_addr_t *addr);

static char addr_str[IPV6_ADDR_MAX_STR_LEN];

static gnrc_netif_handler_t if_handler[] = {
#ifdef MODULE_GNRC_IPV6_NETIF
    { gnrc_ipv6_netif_add, gnrc_ipv6_netif_remove },
#endif
    /* #ifdef MODULE_GNRC_IPV4_NETIF
     *  { ipv4_netif_add, ipv4_netif_remove },
     * #endif ... you get the idea
     */
    { NULL, NULL }
};

static kernel_pid_t ifs[GNRC_NETIF_NUMOF];

void gnrc_netif_init(void)
{
    for (int i = 0; i < GNRC_NETIF_NUMOF; i++) {
        ifs[i] = KERNEL_PID_UNDEF;
    }
}

int gnrc_netif_add(kernel_pid_t pid)
{
    kernel_pid_t *free_entry = NULL;

    for (int i = 0; i < GNRC_NETIF_NUMOF; i++) {
        if (ifs[i] == pid) {
            return 0;
        }
        else if (ifs[i] == KERNEL_PID_UNDEF && !free_entry) {
            free_entry = &ifs[i];
        }
    }

    if (!free_entry) {
        return -ENOMEM;
    }

    *free_entry = pid;

    for (int j = 0; if_handler[j].add != NULL; j++) {
        if_handler[j].add(pid);
    }

    return 0;
}

void gnrc_netif_remove(kernel_pid_t pid)
{
    int i;

    for (i = 0; i < GNRC_NETIF_NUMOF; i++) {
        if (ifs[i] == pid) {
            ifs[i] = KERNEL_PID_UNDEF;

            for (int j = 0; if_handler[j].remove != NULL; j++) {
                if_handler[j].remove(pid);
            }

            return;
        }
    }
}

size_t gnrc_netif_get(kernel_pid_t *netifs)
{
    size_t size = 0;

    for (int i = 0; i < GNRC_NETIF_NUMOF; i++) {
        if (ifs[i] != KERNEL_PID_UNDEF) {
            netifs[size++] = ifs[i];
        }
    }

    return size;
}

bool gnrc_netif_exist(kernel_pid_t pid)
{
    for (int i = 0; i < GNRC_NETIF_NUMOF; i++) {
        if (ifs[i] == pid) {
            return true;
        }
    }
    return false;
}

/** @} */
