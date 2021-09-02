#include "scheduler-store.h"

void
SchedulerStore::onSwitchUpdate (ProbeHeader2 header, std::vector<ProbePayload2> payload) {
	for(ProbePayload2 p: payload){
		switches[header.GetSwid()][p.GetPortId()].maxQueueOccupancy = p.GetMaxQueueDepth();	
	}
}

void 
SchedulerStore::log(void) {
	for(auto s: switches) {
		std::cout << "swid: " << s.first << "\n";
		for(auto p : s.second) {
			std:: cout << "		Port = " << p.first << " Q = " << p.second.maxQueueOccupancy << std::endl;
		}
	}
	std::cout << std::endl;
}