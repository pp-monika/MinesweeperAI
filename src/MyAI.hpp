// ======================================================================
// FILE:        MyAI.hpp
//
// AUTHOR:      Jian Li
//
// DESCRIPTION: This file contains your agent class, which you will
//              implement. You are responsible for implementing the
//              'getAction' function and any helper methods you feel you
//              need.
//
// NOTES:       - If you are having trouble understanding how the shell
//                works, look at the other parts of the code, as well as
//                the documentation.
//
//              - You are only allowed to make changes to this portion of
//                the code. Any changes to other portions of the code will
//                be lost when the tournament runs your code.
// ======================================================================

#ifndef MINE_SWEEPER_CPP_SHELL_MYAI_HPP
#define MINE_SWEEPER_CPP_SHELL_MYAI_HPP

#include "Agent.hpp"
#include <iostream> // temporary use
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <queue>
#include <utility>
#include <bitset>
#include <cmath>
#include <stdlib.h>

using namespace std;


class MyAI : public Agent
{
public:
    MyAI ( int _rowDimension, int _colDimension, int _totalMines, int _agentX, int _agentY );

    Action getAction ( int number ) override;


    // ======================================================================
    // YOUR CODE BEGINS
    // ======================================================================

private:

    struct usingTile {
        int tileX;
        int tileY;
        int number;
    };

    struct tileProb {
        int tileX;
        int tileY;
        double probability;
    };

    struct compareNumber {
        bool operator()(usingTile const& t1, usingTile const& t2) {
            return t1.number > t2.number;
        }
    };

    struct compareProb {
        bool operator()(tileProb const& t1, tileProb const& t2) {
            return t1.probability > t2.probability;
        }
    };

    struct compareMaxProb {
        bool operator()(tileProb const& t1, tileProb const& t2) {
            return t1.probability < t2.probability;
        }
    };

    int flagCount;
    int uncoverCount;

    int leftCoveredX = 1;
    int leftCoveredY = 1;

    bool pqUpdate = false;
    bool workOnFrontier = false;

    // Contains the # of safe neighbors of the coordicate tile (x, y)
    std::vector<vector<int>> effectiveLabels;
    std::priority_queue<usingTile, vector<usingTile>, compareNumber> pq;

    // A vector containing no more than 10 covered frontier
    std::vector<std::pair<int, int>> coveredFrontier;
    std::bitset<20> truthTableRow;
    std::vector<std::bitset<20>> passedAssignments;
    std::queue<std::pair<int, int>> safeCoveredFrontierTile;
    std::queue<std::pair<int, int>> mineCoveredFrontierTile;

    // Return total number of neighboring covered tiles
    int getNumNeighborCovered(int x, int y);
    int getNumFlagNeighbor(int x, int y);

    // Return any tile surrounding a number = 0 tile
    std::pair<int, int> getCoveredNeighbor(int x, int y);
    std::queue<std::pair<int, int>> getAllCoveredNeighbors(int x, int y);
    std::set<std::pair<int, int>> getAllUncoveredFrontiers(int x, int y);
    int findInVector(std::vector<std::pair<int, int>> v, std::pair<int, int> p);
    bool existInQueue(std::queue<std::pair<int, int>> q, std::pair<int, int> p);
    std::vector<std::pair<int, int>> getBoardCoveredTiles();
    std::pair<int, int> getRandomCoveredTile();
    std::pair<int, int> getRandomCoveredFrontierTile();


    
    // Print functions for debugging
    void printQ(std::queue<std::pair<int, int>> q);
    void printVector(std::vector<std::pair<int, int>> v);
    void printVector(std::vector<usingTile> u);
    void printPQ(std::priority_queue<usingTile, vector<usingTile>, compareNumber> pq);
    void printMinProb(std::priority_queue<tileProb, vector<tileProb>, compareProb> tb);
    void printPassedAssignments(std::vector<std::bitset<20>> pa);
    void printEF();
    
    // Update the effective labels of the neighboring tiles when a FLAG action is returned
    void updateEffectiveLabels(int x, int y);

    
    
};


#endif //MINE_SWEEPER_CPP_SHELL_MYAI_HPP
