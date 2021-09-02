#ifndef SCH_TOPO_H
#define SCH_TOPO_H

#include <iostream>
#include <unordered_map>
#include <vector>
#include <stdlib.h>

typedef struct
{
  int swid;
  int portid;
} Entry;

typedef struct {
  std::vector<Entry> path; 
} Path;


class SchedulerTopo
{
  public:
  SchedulerTopo();

  private:
  std::vector<std::vector<Path>> topoPath;   
  std::unordered_map<int, std::string> ips;
};

#endif