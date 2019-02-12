#include<stdio.h>
#include <libvirt/libvirt.h>
#include <stdlib.h>
#include <unistd.h>

struct DomainMemory {
	virDomainPtr domain;
	long memory;
};



struct DomainList {
	virDomainPtr *domains;
};

int LOW_THRESHOLD = 150*1024;
int HIGH_THRESHOLD = 200*1024;

int main(int arg){	
	virConnectPtr conn ;
	char *uri;
	virDomainPtr *domains;
	int num_domains;
	char *domain_name;
	struct DomainMemory excess_domain;
	struct DomainMemory needy_domain;
	struct DomainList list;
	unsigned int flags = VIR_CONNECT_LIST_DOMAINS_RUNNING |
                     VIR_CONNECT_LIST_DOMAINS_ACTIVE;
	int mem_stat_period;
	int stats; 
	conn = virConnectOpen("qemu:///system");
	if (conn == NULL) {
        fprintf(stderr, "Failed to open connection to qemu:///system\n");
        return 1;
    	}

    	uri = virConnectGetURI(conn);

    	fprintf(stdout, "Canonical URI: %s\n", uri);
	free(uri);

	num_domains = virConnectListAllDomains(conn, &domains, flags);
	list.domains = domains;
	printf ("num of domains: %d\n", num_domains);

	while (num_domains>0){
	
	excess_domain.memory = 0;
	needy_domain.memory = 0;

	for (int i = 0; i < num_domains; i++){
		virDomainMemoryStatStruct memstats[VIR_DOMAIN_MEMORY_STAT_NR];
		domain_name=virDomainGetName(list.domains[i]);
		printf("Domain Name: %s\n", domain_name);
		mem_stat_period = virDomainSetMemoryStatsPeriod(list.domains[i],arg, VIR_DOMAIN_AFFECT_CURRENT);
		if (mem_stat_period < 0){
			printf("Error setting balloon collecting period");
			exit(1);
		}
		stats = virDomainMemoryStats(list.domains[i], memstats, VIR_DOMAIN_MEMORY_STAT_NR, 0);
		if(stats== -1){
			printf("Error collecting memory stats for domain");
		}
		printf("%s : %llu MB available\n",virDomainGetName(list.domains[i]),
		       (memstats[VIR_DOMAIN_MEMORY_STAT_AVAILABLE].val)/1024);
		if (memstats[VIR_DOMAIN_MEMORY_STAT_AVAILABLE].val > excess_domain.memory) {
			excess_domain.domain = list.domains[i];
			excess_domain.memory = memstats[VIR_DOMAIN_MEMORY_STAT_AVAILABLE].val;
		}
		if (memstats[VIR_DOMAIN_MEMORY_STAT_AVAILABLE].val < needy_domain.memory ||
		    needy_domain.memory == 0) {
			needy_domain.domain = list.domains[i];
			needy_domain.memory = memstats[VIR_DOMAIN_MEMORY_STAT_AVAILABLE].val;
		}
		printf("Excess domain %s Memory: %d\n", virDomainGetName(excess_domain.domain), excess_domain.memory/1024);
		printf("Need domain %s Memory: %d\n", virDomainGetName(needy_domain.domain), needy_domain.memory/1024);
	}

		if (needy_domain.memory <= LOW_THRESHOLD){
			if (excess_domain.memory >= HIGH_THRESHOLD){

				printf("Removing memory (%ld) from excess domain: %s\n",excess_domain.memory - HIGH_THRESHOLD, virDomainGetName(excess_domain.domain));
				virDomainSetMemory(excess_domain.domain,
							excess_domain.memory - HIGH_THRESHOLD);
				printf("Adding memory (%ld) to needy domain: %s\n", needy_domain.memory + LOW_THRESHOLD*2, virDomainGetName(needy_domain.domain));
				virDomainSetMemory(needy_domain.domain,
					needy_domain.memory + LOW_THRESHOLD*2);			
			}
			
			else{
				needy_domain.memory += HIGH_THRESHOLD;
				printf("Adding memory (%ld) to needy domain: %s\n", needy_domain.memory, virDomainGetName(needy_domain.domain));
				virDomainSetMemory(needy_domain.domain,
							   needy_domain.memory);
			}		
		}
		else if (excess_domain.memory >= HIGH_THRESHOLD){
			printf("Releasing memory (%ld) from excess domain: %s to Host\n",excess_domain.memory - HIGH_THRESHOLD, virDomainGetName(excess_domain.domain));
				virDomainSetMemory(excess_domain.domain,
						   excess_domain.memory - HIGH_THRESHOLD);
		}
		sleep(arg);
		printf("\n\n..Iteration ended..\n\n");
	}

	virConnectClose(conn);
	free(conn);
	return 0;
}
