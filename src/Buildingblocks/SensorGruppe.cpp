#include <vector>
#include <iostream>

#include "SensorGruppe.h"


	SensorGruppe::SensorGruppe(int minimaleDistanz, int maximaleDistanz, int maxSensoren, bool square) {
		this->square = square;
		if (square) {
			int maxDist = random(minimaleDistanz + 1, maximaleDistanz);
			std::cout << "a " << maxDist << std::endl;
			this->maxID = nat_sum(maxDist) * 2;
			int minDist = random(minimaleDistanz, maxDist - 1);
			std::cout << "b " << minDist << std::endl;
			this->minID = nat_sum(minDist) * 2;
			this->maxSensoren = maxSensoren;
			//fillSensorGroup();

			this->updateGenes();
			this->updateRange(1, maxSensoren, minID, maxID);
		}
		else {
			int maxDist = random(minimaleDistanz + 1, maximaleDistanz);
			std::cout << "a " << maxDist << std::endl;
			this->maxID = EigththDiskPixels(maxDist)-1;
			int minDist = random(minimaleDistanz, maxDist - 1);
			std::cout << "b " << minDist << std::endl;
			this->minID = EigththDiskPixels(minDist-1);
			this->maxSensoren = maxSensoren;
			//fillSensorGroup();

			this->updateGenes();
			this->updateRange(1, maxSensoren, minID, maxID);
		}
	}

	void SensorGruppe::fillSensorGroup() { //nur zum spaß??
		this->SensorIDs = {};
		for (size_t i = 0; i < (int)maxSensoren / 3; i++)
		{
			int b = random(0, 3);
			int a = random(0, maxID - minID);
			if (none_of(SensorIDs.begin(), SensorIDs.end(), compare(a)))
			{
				if (b != 0)
				{
					SensorIDs.push_back(a);
				}
			}
		}
	}

	void SensorGruppe::updateRange(int minGroesse, int maxGroesse, int minID, int maxID) {
		minGroesse = minGroesse > maxID - minID ? maxID - minID : minGroesse;
		maxGroesse = maxGroesse > maxID - minID ? maxID - minID : maxGroesse;
		this->groesse = random(minGroesse, maxGroesse);
		this->firstID = random(minID, maxID - groesse);
	}

	void SensorGruppe::updateGenes() {
		std::vector<int> somePrimes = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311, 313, 317, 331, 337, 347, 349, 353, 359, 367, 373, 379, 383, 389, 397, 401, 409, 419, 421, 431, 433, 439, 443, 449, 457, 461, 463, 467, 479, 487, 491, 499, 503, 509, 521, 523, 541, 547, 557, 563, 569, 571, 577, 587, 593, 599, 601, 607, 613, 617, 619, 631, 641, 643, 647, 653, 659, 661, 673, 677, 683, 691, 701, 709, 719, 727, 733, 739, 743, 751, 757, 761, 769, 773, 787, 797, 809, 811, 821, 823, 827, 829, 839, 853, 857, 859, 863, 877, 881, 883, 887, 907, 911, 919, 929, 937, 941, 947, 953, 967, 971, 977, 983, 991, 997 };
		this->DNA = {};
		int Index1 = random(0, somePrimes.size() - 1);
		this->DNA.push_back(somePrimes[Index1]);
		somePrimes[Index1] = 1;
		this->DNA.push_back(somePrimes[random(0, somePrimes.size() - 1)]);
		this->DNA.push_back(random(0, 2048));
	}

	void SensorGruppe::updateGeneOffset(int maxOffset) {
		if (this->DNA.size() == 3) {
			this->DNA[2] = random(0, maxOffset);
		}
		else {
			std::cout << "DNA ist leer. Offset erst nach updateGenes aufrufen." << std::endl;
			std::cout << DNA.size() << std::endl;
		}
	}