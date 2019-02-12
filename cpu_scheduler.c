#include<stdio.h>
#include<libvirt/libvirt.h>

static int USAGE_THRESHOLD = 60;
struct DomainsList{
	virDomainPtr *domains;	
	int num_domains;
}list;

struct DomainStats{
	virDomainPtr domain;
	double usage;
	long unsigned vcpu_time;
	int vcpu_id;
};

int num_cpus=0;
virDomainPtr *dom;


void pinCpus(virDomainPtr *domain, int num_domains, int num_cpus){
	
	printf("Pin CPU\n");
	
	
	unsigned char a ;
	for(int i=0; i<num_domains;){

		for(int j=0; j<num_cpus;j++){
			a = 1<<(j%num_cpus);
			printf("hellosss\n");
			virDomainPinVcpu(dom[i],0,&a,VIR_CPU_MAPLEN(num_cpus));
			printf("%d\n",i);
			i++;
		}	
	}
	printf("Pin Cpu exits");
	
}

unsigned long collectPcpuStats(virConnectPtr conn , int num_cpus){
	virNodeCPUStatsPtr params;
	int nparams = 0;
	unsigned long pcpu_usage_time = 0;
	double pcpu[num_cpus];
	if (virNodeGetCPUStats(conn, num_cpus, NULL, &nparams, 0) == 0 && nparams != 0) {
    		if ((params = malloc(sizeof(virNodeCPUStats) * nparams)) == NULL)
        		goto error;
    		memset(params, 0, sizeof(virNodeCPUStats) * nparams);
		for(int i=0; i<num_cpus; i++)
		{	
			virNodeGetCPUStats(conn, i, params, &nparams, 0);
			for (int i=0; i<nparams ; i++){
	
			if(strcmp(params[i].field, VIR_NODE_CPU_STATS_USER) == 0){
				pcpu[i] = params[i].value;
				pcpu_usage_time += params[i].value;
				
			}
			else if(strcmp(params[i].field, VIR_NODE_CPU_STATS_KERNEL) == 0){
				pcpu[i] = params[i].value;
				pcpu_usage_time += params[i].value;
			}
			printf("pCPUs: %s  usage time: %llu nanosec\n", params[i].field, params[i].value);
			
			}
				
		}
    		
	}
	
	return pcpu_usage_time;
error:
	exit(1);
}

int main(int arg){

	virConnectPtr conn ;
	virNodeCPUStatsPtr cpu_stats;
	virNodeInfo info;
	virDomainPtr *domains;
	double pcpu_avg_usage;
	double vcpu_avg_usage;
	double *cpu_usage;
	unsigned char *cpumaps;
	struct DomainStats *prev_vcpu_stats = calloc(0, sizeof(prev_vcpu_stats));
	struct DomainStats *curr_vcpu_stats = calloc(0, sizeof(prev_vcpu_stats));
	unsigned long prev_pcpu_time, curr_pcpu_time;
	virDomainStatsRecordPtr *records = NULL;
// connect to the Hypervisor
	conn = virConnectOpen("qemu:///system");
	if (conn == NULL) {
        	printf("Error connecting to qemu:///system\n");
        	return 1;
    	}

//get all active running virtual machines
	list.num_domains = virConnectListAllDomains(conn, &domains, VIR_CONNECT_LIST_DOMAINS_RUNNING);
	virConnectListAllDomains(conn, &dom, VIR_CONNECT_LIST_DOMAINS_RUNNING);
	list.domains = domains;
	printf ("num of domains: %d\n", list.num_domains);
	num_cpus = virNodeGetCPUMap(conn, NULL, NULL, 0);
	prev_pcpu_time = collectPcpuStats(conn, num_cpus);
	if (virNodeGetInfo(conn, &info) == -1){
		printf("Error getting node info");
	}
	while (list.num_domains>0){
		curr_pcpu_time = collectPcpuStats(conn, num_cpus);
		pcpu_avg_usage = (curr_pcpu_time - prev_pcpu_time)*100/(arg*1000000000/info.cpus);
		prev_pcpu_time = curr_pcpu_time;
		printf("Average pCPU Usage: %f\n",pcpu_avg_usage);

 //connect to the Hypervisor
		if (virDomainListGetStats(list.domains, VIR_DOMAIN_STATS_VCPU, &records, 0) == -1){
			printf("Error retrieving VCPU stats");		
		}
		for (int i=0; i<list.num_domains; i++){	
			curr_vcpu_stats[i].domain = list.domains[i];
			
			curr_vcpu_stats[i].vcpu_time = records[i]->params[3].value.ul;	
			curr_vcpu_stats[i].vcpu_id = i;
			curr_vcpu_stats[i].usage = 0;	
			printf("Domain: %s %s : %lu\n",virDomainGetName(list.domains[i]), records[i]->params[3].field, curr_vcpu_stats[i].vcpu_time);
			printf("vCPU Id : %d", curr_vcpu_stats[i].vcpu_id);
			printf("Current Vcpu time: %lu\n", curr_vcpu_stats[i].vcpu_time);
			printf("Previous Vcpu time: %lu\n\n", prev_vcpu_stats[i].vcpu_time);
			curr_vcpu_stats[i].usage = (curr_vcpu_stats[i].vcpu_time - prev_vcpu_stats[i].vcpu_time)*100/(arg*1000000000);

			printf("Current Vcpu Usage: %f\n", curr_vcpu_stats[i].usage);
			printf("Previous Vcpu Usage: %f\n\n", prev_vcpu_stats[i].usage);
			prev_vcpu_stats[i].vcpu_time = curr_vcpu_stats[i].vcpu_time;	
			prev_vcpu_stats[i].vcpu_id = curr_vcpu_stats[i].vcpu_id;
			prev_vcpu_stats[i].usage = curr_vcpu_stats[i].usage;
			vcpu_avg_usage += curr_vcpu_stats[i].usage;	
		}
		vcpu_avg_usage = vcpu_avg_usage/list.num_domains;
		pinCpus(list.domains,list.num_domains, num_cpus);
		
		
	
	
	sleep(arg);	
	}
	
	

}
