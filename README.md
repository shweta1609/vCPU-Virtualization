vCPU SCHEDULER
________________________________________________

This program is an implmentation of a vCPU Scheduler used to balance the pinning between pCPU and vCPU. 

PREREQUISITE
________________________________________________

* qemu-kvm and libvirt-bin should be installed
* Verify that hard supports necessary virtualization extensions for KVM
* install uvtool and uvtool-libvirt
* Atleast 4 VM instances
* More than 2GB of host memory

COMPILE
________________________________________________

Go to the home directory of this file and run make. It will compile cpu_scheduler.c into cpu_scheduler in the same directory.

RUN
________________________________________________

To run the file, execute following command in the same directory:
./cpu_scheduler 10
The program expects a run-time input (an integer) which specifies the period after which this scheduler will run. In the above example, my scheduler will run after every 10seconds.


DESCRIPTION
________________________________________________

This algorithm is based on the fact that vCPUs in libvirt are mapped to all physical CPUs in the hypervisor by default. My scheduler aims to  pin vCPUs to pCPUs so that pCPU usage is balanced. It tries to make sure that pCPU usage is balanced and below 40% usage.

The scheduler runs periodically, and in each period it first calculates:

* Individual usage of all the domains for each period
* Usage %age of all the domains periodically, since the start of the algorithm
* Usage %age of the pCPUs
* Based on the usage calculated above, it determines the most busy and the most free pCPU
* With the above available information, scheduler pins more domains to free pCPU as below:
	1. If busy pCPU is above 40% usage threshold
		- vCPU pinned to the free pCPU is swapped with this busy pCPU
	2. Else, Do nothing.


