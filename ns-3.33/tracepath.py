import subprocess
import sys

traceScript = 'scratch/threetree-topotrace.cc {}'
add_path = "\ttopoPath.at({}).at({}).path.push_back(create({},{}));\n"

traces = []
addPathTraces = []

terminals = []

def log():
	print("Total switches: {}".format(len(terminals)) )
	print("Current path tracing count = {}".format(len(traces)))

def processOutput(output, _from, _to):
	trace = {
		'from': _from,
		'to': _to,
		'path': []
	}

	for line in output.split('\n'):
		if line.startswith('PATH'):
			p = [ int(a) for a in line.split(' ')[1:] ]
			trace['path'].append(p)
			addPathTraces.append(add_path.format(_from, _to, p[0], p[1]))
			
	traces.append(trace)	
	log()
	
def runPathTrace(_from, _to):
	result = subprocess.run(['./waf', '--run', traceScript.format("--runtrace=1 --toTerminal={} --fromTerminal={}").format(_to, _from)], stdout=subprocess.PIPE)
	processOutput(result.stdout.decode('utf-8'), _from, _to)

def getTerminalsIps():
	result = subprocess.run(['./waf', '--run', traceScript.format("--dumpterminalips=1")], stdout=subprocess.PIPE)
	ips = []	
	for line in result.stdout.decode('utf-8').split('\n'):
		if line.startswith('DUMPIPS'):
			s = line.split(' ')[1:]
			ips.append((int(s[0]), s[1]))
	return ips

def genCppSource(data, totalSwitches, terminals):
	src = """ 
	#include "scheduler-topo.h"

	Entry create(int swid, int pid) {
	Entry entry;
	entry.swid = swid;
	entry.portid = pid;
	return entry;
	}

	SchedulerTopo::SchedulerTopo() {
	
	"""
	ip_entry = '\tips[{}]="{}";\n'
	init_from = "\ttopoPath.push_back(std::vector<Path>());\n"
	init_to = "\ttopoPath.at({}).push_back(Path());\n"
	
	src_ip = ""
	for entry in terminals:
		src_ip += ip_entry.format(entry[0], entry[1])

	# lets init
	src_init = ""
	for i in range(totalSwitches):
		src_init += init_from
	
	# init init_to
	src_initto = ""
	for i in range(totalSwitches):
		for j in range(totalSwitches):
			src_initto += init_to.format(i)
	

	src_paths = ''.join(addPathTraces)

	constructor_src = "{}{}{}{}".format(src_ip, src_init, src_initto, src_paths)
	
	sch_topo_cpp = "{}{}\n}}".format(src, constructor_src)

	with open('scheduler-topo.cc', "w") as cpp:
		cpp.write(sch_topo_cpp)



terminals = getTerminalsIps()
for i in range(len(terminals)):
	for j in range(len(terminals)):
		runPathTrace(i, j)


genCppSource(traces, len(terminals), terminals)