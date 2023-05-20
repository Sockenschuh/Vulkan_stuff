#ifndef SENSORGRUPPE_H
#define SENSORGRUPPE_H

#include <vector>
#include <iostream>

#include "Simulation_base.h"

class SensorGruppe {
public:
  int maxID;
  int minID;
  int maxSensoren;
  std::vector<int> SensorIDs;

  std::vector<int> DNA;
  int groesse;
  int firstID;

  bool square;

  SensorGruppe(int minimaleDistanz, int maximaleDistanz, int maxSensoren, bool square);

  void fillSensorGroup();
  void updateRange(int minGroesse, int maxGroesse, int minID, int maxID);
  void updateGenes();
  void updateGeneOffset(int maxOffset);
};

#endif // SENSORGRUPPE_H