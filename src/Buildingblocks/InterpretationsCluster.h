#ifndef INT_H
#define INT_H
#include <vector>

#include "SensorGruppe.h"

class InterpretationsCluster
{
public:
	int wertigkeit;
	int groesse;
	std::vector<int> SensorGruppenIDs;
	std::vector<std::vector<int>> ClusterDNAs;

	std::vector<SensorGruppe*> UsedGroups;

	bool square;

	InterpretationsCluster(std::vector<SensorGruppe*> SensorGruppen, std::vector<int> toUseIDs, bool square);

	static uint32_t SetWertigkeiten(std::vector<InterpretationsCluster*> ActiveClusters);
};

#endif