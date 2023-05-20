#ifndef SP_H
#define SP_H
#include <vector>
#include <cstdint>
#include <iostream>

#include "InterpretationsCluster.h"

class Spezies
{
public:
    std::vector<int> DNA;
    std::vector<InterpretationsCluster*> Brain;
    uint32_t WishAmount;
    uint32_t StayAmount;

    Spezies(std::vector<InterpretationsCluster*> brain, int maxWish, int maxStay);
    void updateWishAmount(uint32_t minWish, uint32_t maxWish);
    void updateStayAmount(uint32_t minStay, uint32_t maxStay);
    void updateGenes();
    void updateGeneOffset(int maxOffset);
};

#endif