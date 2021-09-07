#include "scheduler-store.h"

void
SchedulerStore::onSwitchUpdate (ProbeHeader2 header, std::vector<ProbePayload2> payload) {
	for(ProbePayload2 p: payload){
		switches[header.GetSwid()][p.GetPortId()].maxQueueOccupancy = p.GetMaxQueueDepth();	
	}
}

void
SchedulerStore::log(uint16_t swid) {
	std::cout << "Switch: "<< (int)swid << "\n";
	for(auto p: switches[swid]) {
			std:: cout << "		Port = " << p.first << " Q = " << p.second.maxQueueOccupancy << std::endl;
	}
}

std::unordered_map<int, int>
SchedulerStore::tracePath(uint16_t a, uint16_t b) {
	std::unordered_map<int, int> effectiveQs;
	int q = 0;
	for(int i=topo.minNodeId; i<=topo.maxNodeId; i++) {
		if(i == a) continue;

		q = 0;
		for(auto path: topo.topoPath.at(a).at(b).path) {
			 q += switches[path.swid][path.portid].maxQueueOccupancy;
		}
		effectiveQs[i] = q;
	}

	//  for(auto x: effectiveQs) {
	// 	 if(x.second > 0)
	// 	 std::cout << x.first << "=" << x.second << std::endl;
	//  }
	 return effectiveQs;
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