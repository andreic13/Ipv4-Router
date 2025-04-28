#include "queue.h"
#include "lib.h"
#include "protocols.h"
#include <arpa/inet.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/if_ether.h>

//tabela de rutare
struct route_table_entry *rtable;
int rtable_len;

//tabela arp/mac
struct arp_table_entry *arp_table;
int arp_table_len;

//Functie de comparare pt qsort
int cmpfunc(const void *a, const void *b) {
	struct route_table_entry *entry1 = (struct route_table_entry *)a;
	struct route_table_entry *entry2 = (struct route_table_entry *)b;

	uint32_t prefix1 = ntohl(entry1->prefix);
	uint32_t prefix2 = ntohl(entry2->prefix);
	uint32_t mask1 = ntohl(entry1->mask);
	uint32_t mask2 = ntohl(entry2->mask);

	//initial dupa masca, apoi dupa prefix
	if (mask1 < mask2) {
		return -1;
	} else if (mask1 > mask2) {
		return 1;
	} else if (mask1 == mask2){
		if (prefix1 < prefix2) {
			return -1;
		} else if (prefix1 > prefix2) {
			return 1;
		} else {
			return 0;
		}
	}

	return 0;
}

//Gaseste cea mai buna ruta, binar -> eficient
struct route_table_entry *get_best_route(uint32_t ip_dest) {
	int left = 0;
	int right = rtable_len - 1;
	int mid = 0;

	struct route_table_entry *candidate = NULL;

	while (left <= right) {
		mid = left + (right - left) / 2;

		if (ntohl(rtable[mid].prefix) == (ntohl(ip_dest) & ntohl(rtable[mid].mask))) {
			if (candidate == NULL || (ntohl(rtable[mid].mask) > ntohl(candidate->mask))) {
				candidate = &rtable[mid];
			}
			left = mid + 1; //caut in dreapta o masca mai mare
		} else if (ntohl(rtable[mid].prefix) < (ntohl(ip_dest) & ntohl(rtable[mid].mask))) {
			left = mid + 1;
		} else {
			right = mid - 1;
		}
	}

	return candidate;
}

//Gaseste adresa mac corespunzatoare unui IP (functie din LAB4)
struct arp_table_entry *get_arp_entry(uint32_t given_ip) {
	for (int i = 0; i < arp_table_len; i++) {
		if (given_ip == arp_table[i].ip)
			return &arp_table[i];
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	char buf[MAX_PACKET_LEN];
	int interface;
	size_t len;

	// Do not modify this line
	init(argc - 2, argv + 2);

	//parsare tabela de rutare + tabela ARP
	rtable = malloc(sizeof(struct route_table_entry) * 80000);
	DIE(rtable == NULL, "memory");

	arp_table = malloc(sizeof(struct arp_table_entry) * 100);
	DIE(arp_table == NULL, "memory");

	rtable_len = read_rtable(argv[1], rtable);
	arp_table_len = parse_arp_table("arp_table.txt", arp_table);

	//sortare rtable pt binary search:
	qsort(rtable, rtable_len, sizeof(struct route_table_entry), cmpfunc);

	while (1) {
		interface = recv_from_any_link(buf, &len);
		DIE(interface < 0, "recv_from_any_link");

		struct ether_header *eth_hdr = (struct ether_header *) buf;
		/* Note that packets received are in network order,
		any header field which has more than 1 byte will need to be conerted to
		host order. For example, ntohs(eth_hdr->ether_type). The oposite is needed when
		sending a packet on the link, */
		
		struct iphdr *ip_hdr = (struct iphdr *)(buf + sizeof(struct ether_header));

		// Verfic daca pachetul e IPv4
		if (eth_hdr->ether_type != ntohs(0x0800)) {
			printf("Packet not IPv4\n");
			continue;
		}
		
		struct icmphdr *icmp_hdr = (struct icmphdr *)(buf + sizeof(struct ether_header) + sizeof(struct iphdr));

		// Verific daca sunt destinatia -- 1.1 + Trimit ICMP "Echo reply" daca sunt
		if (ip_hdr->daddr == inet_addr(get_interface_ip(interface))) {
			//modific structura ICMP
			icmp_hdr->type = 0;
			icmp_hdr->code = 0;
			icmp_hdr->checksum = 0;
			icmp_hdr->checksum = htons(checksum((uint16_t *)icmp_hdr, sizeof(struct icmphdr)));
			//modific structura IP pentru ca encapsuleaza ICMP
			uint32_t aux = ip_hdr->daddr;
			ip_hdr->daddr = ip_hdr->saddr; //trimit inapoi la sursa
			ip_hdr->saddr = aux;
			ip_hdr->protocol = 1; //encapsuleaza protocol ICMP
			ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr));
			ip_hdr->check = 0;
			ip_hdr->check = htons(checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)));

			send_to_link(interface, buf, sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr));
			continue;
		} 
		
		// Verific checksum -- 1.2
		if (checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)) != 0) {
			printf("Wrong checksum\n");
			continue;
		}

		//Verific + Update TTL -- 1.3 + Trimit ICMP "Time exceeded" daca expira ttl
		if(ip_hdr->ttl <= 1) {
			//S-a terminat TTL
			//modific structura ICMP
			icmp_hdr->type = 11;
			icmp_hdr->code = 0;
			icmp_hdr->checksum = 0;
			icmp_hdr->checksum = htons(checksum((uint16_t *)icmp_hdr, sizeof(struct icmphdr)));
			//modific structura IP pentru ca encapsuleaza ICMP
			uint32_t aux = ip_hdr->daddr;
			ip_hdr->daddr = ip_hdr->saddr; //trimit inapoi la sursa
			ip_hdr->saddr = aux;
			ip_hdr->protocol = 1; //encapsuleaza protocol ICMP
			ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr));
			ip_hdr->check = 0;
			ip_hdr->check = htons(checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)));

			send_to_link(interface, buf, sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr));
			continue;
		} else {
			//Update TTL
			ip_hdr->ttl -= 1;
		}

		//Caut cea mai buna ruta in rtable -- 1.4 + Trimit ICMP "Destination unreachable" daca nu am ruta
		struct route_table_entry *best_route = get_best_route(ip_hdr->daddr);
		if (best_route == NULL) {
			//modific structura ICMP
			icmp_hdr->type = 3;
			icmp_hdr->code = 0;
			icmp_hdr->checksum = 0;
			icmp_hdr->checksum = htons(checksum((uint16_t *)icmp_hdr, sizeof(struct icmphdr)));
			//modific structura IP pentru ca encapsuleaza ICMP
			uint32_t aux = ip_hdr->daddr;
			ip_hdr->daddr = ip_hdr->saddr; //trimit inapoi la sursa
			ip_hdr->saddr = aux;
			ip_hdr->protocol = 1; //encapsuleaza protocol ICMP
			ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr));
			ip_hdr->check = 0;
			ip_hdr->check = htons(checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)));

			send_to_link(interface, buf, sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr));
			continue;
		}

		//Actualizare checksum -- 1.5
		ip_hdr->check = 0;
		ip_hdr->check = htons(checksum((uint16_t *)ip_hdr, sizeof(struct iphdr)));

		//Rescriere L2 -- 1.6
		struct arp_table_entry *dest_mac = get_arp_entry(best_route->next_hop);

		if (dest_mac == NULL) {
			printf("No mac entry\n");
			continue;
		}

		memcpy(eth_hdr->ether_dhost, dest_mac->mac, ETH_ALEN);
		get_interface_mac(best_route->interface, eth_hdr->ether_shost);

		//Trimitere pachet -- 1.7
		send_to_link(best_route->interface, buf, len);
	}	
}

