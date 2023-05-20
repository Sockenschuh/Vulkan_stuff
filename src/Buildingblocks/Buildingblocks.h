#ifndef B_H
#define B_H
#include <iostream>
#include <vector>

#include "Spezies.h"

class Buildingblocks {
public:
	std::vector<SensorGruppe*> SensorSet;
	std::vector<InterpretationsCluster*> ClusterSet;
	std::vector<Spezies*> SpeziesSet;
	std::vector<InterpretationsCluster*> ActiveCluster;

	std::vector<int> SensorGruppenRawData;

	uint32_t maxCombiSum;
	int absoluteMinimaleDistanz;
	int absoluteMaximaleDistanz;
	bool square;

	void standard_start(int SensorMenge, int minimaleDistanz, int maximaleDistanz, int maxSensoren, int ClusterMenge, int maxGruppen, int SpeziesMenge, int maxWertigkeiten, int maxWish, int maxStay, bool square = true);

	void genSensorGruppen(int Menge, int minimaleDistanz, int maximaleDistanz, int maxSensoren, bool square);

	void genClusters(int Menge, int maxGruppen, bool square);

	void genSpeziesSet(int Menge, int maxWertigkeiten, int maxWish, int maxStay);
};

#endif