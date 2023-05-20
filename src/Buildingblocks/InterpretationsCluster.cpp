#pragma once //Interpretationscluster

#include "InterpretationsCluster.h"



		InterpretationsCluster::InterpretationsCluster(std::vector<SensorGruppe*> SensorGruppen, std::vector<int> toUseIDs, bool square) {
		/*for (size_t i = 0; i < SensorGruppen.size(); i++)
		{
			for (size_t j = 0; j < SensorGruppen[i]->SensorIDs.size(); j++)
			{
				ClusterIDs.push_back(SensorGruppen[i]->SensorIDs[j]);
				this->groesse += 1;
			}
		}*/
		this->square = square;
		UsedGroups = {};

		this->groesse = 0;

		if (square) {
			for (size_t i = 0; i < SensorGruppen.size(); i++)
			{
				ClusterDNAs.push_back({ SensorGruppen[i]->DNA[0], SensorGruppen[i]->DNA[1], SensorGruppen[i]->DNA[2], SensorGruppen[i]->firstID, SensorGruppen[i]->groesse,  SensorGruppen[i]->maxID });
				SensorGruppenIDs.push_back(toUseIDs[i]);

				UsedGroups.push_back(SensorGruppen[i]);
				for (int j = 0; j < SensorGruppen[i]->groesse; j++)
				{
					int ID = (SensorGruppen[i]->DNA[0] * j + SensorGruppen[i]->DNA[2]) % (SensorGruppen[i]->maxID - SensorGruppen[i]->firstID) + SensorGruppen[i]->firstID;
					//cout << "ID:" << ID << "<--- " << j << endl;
					int dist = int(floor((sqrt(8 * (ID + 1) + 1) - 1) / 2));
					int position = ID - nat_sum(dist) + 1;
					if (ID == 0 || ID == dist) this->groesse += 4;
					else this->groesse += 8;
				}
			}
		}
		else {
			for (size_t i = 0; i < SensorGruppen.size(); i++)
			{
				ClusterDNAs.push_back({ SensorGruppen[i]->DNA[0], SensorGruppen[i]->DNA[1], SensorGruppen[i]->DNA[2], SensorGruppen[i]->firstID, SensorGruppen[i]->groesse,  SensorGruppen[i]->maxID });
				SensorGruppenIDs.push_back(toUseIDs[i]);

				UsedGroups.push_back(SensorGruppen[i]);
				for (int j = 0; j < SensorGruppen[i]->groesse; j++)
				{
					int ID = (SensorGruppen[i]->DNA[0] * j + SensorGruppen[i]->DNA[2]) % (SensorGruppen[i]->maxID - SensorGruppen[i]->firstID) + SensorGruppen[i]->firstID;
					//cout << "ID:" << ID << "<--- " << j << endl;
					int dist = aproxDiskRangeEigth(ID);
					int position = ID - EigththDiskPixels(dist-1);

					std::cout << "ID: " << ID << "| dist: " << dist << "| position: " << position << std::endl;

					if (ID == 0 || (position/(diskPixels(dist)-(diskPixels(dist-1)-1)))== 1.0f/8.0f) this->groesse += 4;
					else this->groesse += 8;
				}
			}
		}

	}

	

	uint32_t InterpretationsCluster::SetWertigkeiten(std::vector<InterpretationsCluster*> ActiveClusters) {

		for (int n = ActiveClusters.size(); n > 1; --n) { // BubbleSort
			for (int i = 0; i < n - 1; ++i) {
				if (ActiveClusters[i]->groesse > ActiveClusters[i + 1]->groesse) {
					//std::iter_swap(ActiveClusters.begin() + i, ActiveClusters.begin() + i+1);
					std::swap(ActiveClusters[i], ActiveClusters[i + 1]);
				}
			}
		}

		int Wert = 1;

		for (InterpretationsCluster* cluster : ActiveClusters) {
			cluster->wertigkeit = Wert;
			Wert *= (cluster->groesse + 1);
		}

		return Wert;
	}