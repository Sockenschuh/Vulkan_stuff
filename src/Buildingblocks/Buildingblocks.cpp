#pragma once
#include <iostream>
#include <vector>
#include "Buildingblocks.h"


	void Buildingblocks::standard_start(int SensorMenge, int minimaleDistanz, int maximaleDistanz, int maxSensoren, int ClusterMenge, int maxGruppen, int SpeziesMenge, int maxWertigkeiten, int maxWish, int maxStay, bool square)
	{
		this->SpeziesSet = {};
		this->ClusterSet = {};
		this->SensorSet = {};
		this->ActiveCluster = {};
		this->square = square;
		genSensorGruppen(SensorMenge, minimaleDistanz, maximaleDistanz, maxSensoren, square);
		genClusters(ClusterMenge, maxGruppen, square);
		genSpeziesSet(SpeziesMenge, maxWertigkeiten, maxWish, maxStay);
	}

	void Buildingblocks::genSensorGruppen(int Menge, int minimaleDistanz, int maximaleDistanz, int maxSensoren, bool square)
	{
		this->absoluteMinimaleDistanz = 10;
		this->absoluteMaximaleDistanz = 0;
		SensorSet = {};
		SensorGruppenRawData = {};
		for (size_t i = 0; i < Menge; i++)
		{
			SensorGruppe* a = new SensorGruppe(minimaleDistanz, maximaleDistanz, maxSensoren, square);

			if (a->maxID > this->absoluteMaximaleDistanz) {
				this->absoluteMaximaleDistanz = a->maxID;
			}
			if (a->minID < this->absoluteMinimaleDistanz) {
				this->absoluteMinimaleDistanz = a->minID;
			}

			SensorSet.push_back(a);
			std::vector<int> rawdata = { a->DNA[0], a->DNA[1], a->DNA[2], a->firstID, a->groesse, a->maxID };
			for (int x = 0; x < rawdata.size(); x++) {
				this->SensorGruppenRawData.push_back(rawdata[x]);
			}
		}
	}

	void Buildingblocks::genClusters(int Menge, int maxGruppen, bool square) {
		for (size_t i = 0; i < Menge; i++)
		{
			std::vector<int> usedSensors = {};
			std::vector<SensorGruppe*> toUse = {};
			std::vector<int> toUseIDs = {};
			for (size_t j = 0; j < maxGruppen; j++)
			{
				do {
					if (random(0, 100) % 2 == 0) {
						int Index = random(0, SensorSet.size() - 1);
						if (none_of(usedSensors.begin(), usedSensors.end(), compare(Index))) {
							toUse.push_back(SensorSet[Index]);
							toUseIDs.push_back(Index);
							usedSensors.push_back(Index);
						}
					}
				} while (toUse.size() == 0);
			}
			InterpretationsCluster* a = new InterpretationsCluster(toUse, toUseIDs, square);
			ClusterSet.push_back(a);
		}
	}

	void Buildingblocks::genSpeziesSet(int Menge, int maxWertigkeiten, int maxWish, int maxStay) {
		ActiveCluster = {};
		std::vector<int> usedClusters = {};
		for (size_t i = 0; i < Menge; i++)
		{
			std::vector<InterpretationsCluster*> ClusterstoUse = {};
			for (size_t j = 0; j < maxWertigkeiten; j++)
			{
				do {
					if (random(0, 100) % 2 == 0) {
						int Index = random(0, ClusterSet.size() - 1);
						if (none_of(usedClusters.begin(), usedClusters.end(), compare(Index))) {
							ClusterstoUse.push_back(ClusterSet[Index]);
							usedClusters.push_back(Index);
							ActiveCluster.push_back(ClusterSet[Index]);
						}
					}
				} while (ClusterstoUse.size() == 0);
			}
			//SpeziesSet.push_back(new Spezies(ClusterstoUse, maxWish, maxStay));
			ActiveCluster = ClusterSet;
			SpeziesSet.push_back(new Spezies(ClusterSet, maxWish, maxStay));
		}

		
		
		this->maxCombiSum = InterpretationsCluster::SetWertigkeiten(ActiveCluster);
	}