#ifndef __CR_NETFILTER_H__
#define __CR_NETFILTER_H__

struct inet_sk_desc;
int nf_lock_connection(struct inet_sk_desc *);
int nf_unlock_connection(struct inet_sk_desc *);

struct inet_sk_info;
int nf_unlock_connection_info(struct inet_sk_info *);
#endif
