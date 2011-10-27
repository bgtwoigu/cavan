#pragma once

// Fuang.Cao <cavan.cfa@gmail.com> Thu Apr 21 10:08:25 CST 2011

#include <sys/socket.h>
#include <bits/sockaddr.h>
#include <linux/netlink.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define NETWORK_TIMEOUT_VALUE	5
#define NETWORK_RETRY_COUNT		5
#define ROUTE_TABLE_SIZE		16
#define MAC_ADDRESS_LEN			6

#pragma pack(1)
struct mac_header
{
	u8 dest_mac[6];
	u8 src_mac[6];
	u16 protocol_type;

	u8 data[0];
};

struct ip_header
{
	u8 version			:4;
	u8 header_length	:4;
	u8 service_type;
	u16 total_length;

	u32 flags			:19;
	u32 piece_offset	:13;

	u8 ttl;
	u8 protocol_type;
	u16 header_checksum;

	u32 src_ip;
	u32 dest_ip;

	u8 data[0];
};

struct udp_header
{
	u16 src_port;
	u16 dest_port;

	u16 udp_length;
	u16 udp_checksum;

	u8 data[0];
};

struct tcp_header
{
	u16 src_port;
	u16 dest_port;

	u32 sequence;
	u32 sck_sequence;

	u16 header_length	:4;
	u16 reserve			:6;
	u16 TCP_URG			:1;
	u16 TCP_ACK			:1;
	u16 TCP_PSH			:1;
	u16 TCP_RST			:1;
	u16 TCP_SYN			:1;
	u16 TCP_FIN			:1;
	u16 window_size;

	u16 checksum;
	u16 urgent_pointer;

	u8 data[0];
};

struct arp_header
{
	u16 hardware_type;
	u16 protocol_type;
	u8 hardware_addrlen;
	u8 protocol_addrlen;
	u16 op_code;
	u8 src_mac[6];
	u32 src_ip;
	u8 dest_mac[6];
	u32 dest_ip;
};

struct icmp_header
{
	u8 type;
	u8 code;
	u16 checksum;

	u8 data[0];
};

struct dhcp_header
{
	u8 opcode;
	u8 htype;
	u8 hlen;
	u8 hops;

	u32 transction_id;

	u16 seconds;
	u16 flags;

	u32 ciaddr;
	u32 yiaddr;
	u32 siaddr;
	u32 giaddr;

	u8 chaddr[16];
	u8 sname[64];
	u8 file[128];

	u8 options[0];
};

struct udp_pseudo_header
{
	u32 src_ip;
	u32 dest_ip;

	u8 zero;
	u8 protocol;
	u16 udp_length;
};
#pragma pack()

struct cavan_route_node
{
	void *data;
	u8 mac_addr[MAC_ADDRESS_LEN];
	u32 ip_addr;
};

struct cavan_route_table
{
	struct cavan_route_node **route_table;
	int table_size;
};

ssize_t sendto_select(int sockfd, int retry, const void *buff, size_t len, const struct sockaddr_in *remote_addr);
ssize_t select_receive(int sockfd, long timeout, void *buff, size_t bufflen, struct sockaddr_in *remote_addr, socklen_t *addr_len);
ssize_t sendto_receive(int sockfd, long timeout, int retry, const void *send_buff, ssize_t sendlen, void *recv_buff, ssize_t recvlen, struct sockaddr_in *remote_addr, socklen_t *addr_len);
ssize_t recvform_noblock(int sockfd, long timeout, void *buff, size_t recvlen, struct sockaddr_in *remote_addr, socklen_t *addr_len);

const char *mac_protocol_type_tostring(int type);
const char *ip_protocol_type_tostring(int type);
void show_mac_header(struct mac_header *hdr);
void show_ip_header(struct ip_header *hdr, int simple);
void show_tcp_header(struct tcp_header *hdr);
void show_udp_header(struct udp_header *hdr);
void show_arp_header(struct arp_header *hdr, int simple);
void show_dhcp_header(struct dhcp_header *hdr);
void show_icmp_header(struct icmp_header *hdr);

int cavan_route_table_init(struct cavan_route_table *table, size_t table_size);
void cavan_route_table_uninit(struct cavan_route_table *table);
int cavan_route_table_insert_node(struct cavan_route_table *table, struct cavan_route_node *node);
struct cavan_route_node **cavan_find_route_by_mac(struct cavan_route_table *table, u8 *mac);
struct cavan_route_node **cavan_find_route_by_ip(struct cavan_route_table *table, u32 ip);
int cavan_route_table_delete_node(struct cavan_route_table *table, struct cavan_route_node *node);
int cavan_route_table_delete_by_mac(struct cavan_route_table *table, u8 *mac);
int cavan_route_table_delete_by_ip(struct cavan_route_table *table, u32 ip);

u16 udp_checksum(struct ip_header *ip_hdr);

void inet_sockaddr_init(struct sockaddr_in *addr, const char *ip_address, u16 port);
int inet_create_tcp_link(const char *ip_address, u16 port);
int inet_create_service(int type, u16 port);
int inet_create_tcp_service(u16 port);
void inet_show_sockaddr(const struct sockaddr_in *addr);

static inline int inet_socket(int type)
{
	return socket(AF_INET, type, 0);
}

static inline int inet_bind(int sockfd, const struct sockaddr_in *addr)
{
	return bind(sockfd, (const struct sockaddr *)addr, sizeof(*addr));
}

static inline int inet_connect(int sockfd, const struct sockaddr_in *addr)
{
	return connect(sockfd, (const struct sockaddr *)addr, sizeof(*addr));
}

static inline int inet_accept(int sockfd, struct sockaddr_in *addr, socklen_t *addrlen)
{
	*addrlen = sizeof(struct sockaddr_in);

	return accept(sockfd, (struct sockaddr *)addr, addrlen);
}

static inline ssize_t inet_sendto(int sockfd, const void *buff, size_t size, int flags, const struct sockaddr_in *addr)
{
	return sendto(sockfd, buff, size, flags, (const struct sockaddr *)addr, sizeof(*addr));
}

static inline ssize_t inet_recvfrom(int sockfd, void *buff, size_t size, int flags, struct sockaddr_in *addr, socklen_t *addrlen)
{
	*addrlen = sizeof(struct sockaddr_in);

	return recvfrom(sockfd, buff, size, flags, (struct sockaddr *)addr, addrlen);
}

static inline int inet_create_udp_service(u16 port)
{
	return inet_create_service(SOCK_DGRAM, port);
}

