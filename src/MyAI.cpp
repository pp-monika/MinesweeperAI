// ======================================================================
// FILE:        MyAI.cpp
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

#include "MyAI.hpp"

MyAI::MyAI ( int _rowDimension, int _colDimension, int _totalMines, int _agentX, int _agentY ) : Agent()
{
    // ======================================================================
    // YOUR CODE BEGINS
    // ======================================================================

    rowDimension = _rowDimension;
    colDimension = _colDimension;
    totalMines   = _totalMines;
    agentX = _agentX + 1;
    agentY = _agentY + 1;

    
    // Initialize vector with -2 (in liu with NULL)
    // 1 size larger to match the 1-start system of board coordinate
    std::vector< vector<int> > temp(colDimension + 1, vector<int>(rowDimension + 1, -2));
    effectiveLabels = temp;

    flagCount = 0;
    uncoverCount = 1;


};

Agent::Action MyAI::getAction( int number )
{    
    // If number = -1, skip
    if (number == 0) {

        effectiveLabels[agentX][agentY] = -1;
        pq.push(usingTile{agentX, agentY, -1});

    } else if (number != -1) {

        // Previous action is not FLAG/UNFLAG
        // aka. previou action is UNCOVER with number > 0
        effectiveLabels[agentX][agentY] = getNumNeighborCovered(agentX, agentY) - (number - getNumFlagNeighbor(agentX, agentY));
        pq.push(usingTile{agentX, agentY, getNumNeighborCovered(agentX, agentY) - (number - getNumFlagNeighbor(agentX, agentY))});
    }

    // If all mines are FLAGGED, UNCOVER the rest

    if (uncoverCount < (rowDimension * colDimension - totalMines)) {
        for (int i = leftCoveredX; i <= colDimension; i++) {
            for (int j = leftCoveredY; j <= rowDimension; j++) {
                    
                if (effectiveLabels[i][j] == -2) {
                    leftCoveredX = i;
                    leftCoveredY = j;
                    uncoverCount++;
                        
                    agentX = i;
                    agentY = j;

                    return {UNCOVER, i - 1, j - 1};
                }

                if (j == rowDimension) {
                    leftCoveredY = 1;
                }
            }
        }
            
        return {LEAVE, 0, 0};
    }

    // If all safe tiles are UNCOVERED, FLAG the rest
    if (uncoverCount == rowDimension * colDimension - totalMines) {

        for (int i = leftCoveredX; i <= colDimension; i++) {
            for (int j = leftCoveredY; j <= rowDimension; j++) {
                    
                if (effectiveLabels[i][j] == -2) {
                    leftCoveredX = i;
                    leftCoveredY = j;
                    flagCount++;
                        
                    agentX = i;
                    agentY = j;

                    return {FLAG, i - 1, j - 1};
                }

                if (j == rowDimension) {
                    leftCoveredY = 1;
                }
            }
        }

        if (flagCount == totalMines) {
            return {LEAVE, 0, 0};
        }
    }


    // If model checking says that there is a sure mine tile still covered, FLAG all
    if (mineCoveredFrontierTile.size() > 0) {
        
        std::pair<int, int> current = mineCoveredFrontierTile.front();
        mineCoveredFrontierTile.pop();
        agentX = current.first;
        agentY = current.second;
        flagCount++;
        coveredFrontier.erase(coveredFrontier.begin() + findInVector(coveredFrontier, current));
        effectiveLabels[current.first][current.second] = -3;
        return {FLAG, current.first - 1, current.second - 1};
    }

    // If model checking says that there is a sure safe tile still covered, UNCOVER all
    if (safeCoveredFrontierTile.size() > 0) {
        std::pair<int, int> current = safeCoveredFrontierTile.front();
        safeCoveredFrontierTile.pop();
        agentX = current.first;
        agentY = current.second;
        uncoverCount++;
        updateEffectiveLabels(current.first, current.second);
        coveredFrontier.erase(coveredFrontier.begin() + findInVector(coveredFrontier, current));
        return {UNCOVER, current.first - 1, current.second - 1};
    }

    while (true) {

        if (pq.empty() == true) {


            // If PQ is empty, we can only guess. Make a random moves by uncovering one of the remaining covered tiles.
            std::pair<int, int> randomCoveredTiles = getRandomCoveredTile();

            updateEffectiveLabels(randomCoveredTiles.first, randomCoveredTiles.second);
            agentX = randomCoveredTiles.first;
            agentY = randomCoveredTiles.second;
            uncoverCount++;

            // If the randomized tile exist in the coveredFrontier, erase it.
            int randomIndex = findInVector(coveredFrontier, randomCoveredTiles);
            if (randomIndex != -1) {
                coveredFrontier.erase(coveredFrontier.begin() + randomIndex);
            }

            return {UNCOVER, randomCoveredTiles.first - 1, randomCoveredTiles.second - 1};

        }


        /* 
            Get the tile with the least effective number.
            -1 means all neighbor safe.
            0 means tile is mine.
            1+ means # of safe neighbor tiles = effectiveLevel
        */
        usingTile minEffLabel = pq.top();

        if (minEffLabel.number == -1) {

            // std::cout << "All neighbors are safe" << std::endl;

            // Safe option found, if pqUpdate was update in the last action, reset to false.
            pqUpdate = false;

            std::pair<int, int> safeN = getCoveredNeighbor(minEffLabel.tileX, minEffLabel.tileY);

            // If there is an uncovered neighbor left, aka. (x, y) > (0, 0)
            if (safeN.first != 0 && safeN.second != 0) {

                // If there is only 1 covered neighbors left, remove from pq because next action will UNCOVER it.
                if (getNumNeighborCovered(minEffLabel.tileX, minEffLabel.tileY) == 1) {
                    pq.pop();
                }

                agentX = safeN.first;
                agentY = safeN.second;
                uncoverCount++;
                updateEffectiveLabels(safeN.first, safeN.second);

                int safeIndex = findInVector(coveredFrontier, safeN);
                if (safeIndex != -1) {
                    coveredFrontier.erase(coveredFrontier.begin() + safeIndex);
                }

                return {UNCOVER, safeN.first - 1, safeN.second - 1};

            } else {
                    
                // Else: There is no more covered neighbor
                // Pop this tile from the PQ. Go back to while(true) loop to find the suitable action
                pq.pop();

            }

        } else if (minEffLabel.number == 0) {

            // std::cout << "All neighbors are tiles" << std::endl;

            // Safe option found, if pqUpdate was update in the last action, reset to false.
            pqUpdate = false;

            // A tile with number != 0 but number = remaining covered tile (aka. all neighbors are tiles)
            // FLAG
            std::pair<int, int> mineTile = getCoveredNeighbor(minEffLabel.tileX, minEffLabel.tileY);            

            // If there is an uncovered neighbor left, aka. (x, y) > (0, 0)
            if (mineTile.first != 0 && mineTile.second != 0) {
                
                if (getNumNeighborCovered(minEffLabel.tileX, minEffLabel.tileY) == 1) {
                    pq.pop();
                }

                flagCount++;

                // -3 means FLAGGED
                effectiveLabels[mineTile.first][mineTile.second] = -3;

                int mineIndex = findInVector(coveredFrontier, mineTile);
                if (mineIndex != -1) {
                    coveredFrontier.erase(coveredFrontier.begin() + mineIndex);
                }

                return {FLAG, mineTile.first - 1, mineTile.second - 1};

            } else {

                // Else: all neighbor tiles are UNCOVERED
                // Pop from PQ and return to while(true) loop to find the suitable action
                pq.pop();
                    
            }
            
        } else {

            // There is a mine, and there are more tiles to be uncovered than the mine numbers.
            // aka. unsure territory.

            // If the PQ update is not performed in the last action, update the PQ to reflect current effectiveLabel
            if (pqUpdate == false) {

                // Indicate to the next action that the update was done in the previous action.
                pqUpdate = true;
            

                    //std::cout << "more tiles to be uncovered than the mine numbers -- REFRESH" << std::endl;

                    // A queue use to temporarily store tiles in PQ, for restoring PQ after opration is done.
                    std::priority_queue<usingTile, vector<usingTile>, compareNumber> tempQueue;

                    // Update number (according to EffectiveLabel) for all tiles in PQ
                    while (pq.empty() != true) {
                        usingTile tempTile = pq.top();
                        pq.pop();

                        // Replace number by effectiveLabel
                        if (tempTile.number != effectiveLabels[tempTile.tileX][tempTile.tileY]) {
                            tempTile.number = effectiveLabels[tempTile.tileX][tempTile.tileY];
                        }
                        
                        // If updated number is equal to the # of covered neighbors, then all neighbor is safe.
                        // Set this tile's number to -1.
                        if (tempTile.number == getNumNeighborCovered(tempTile.tileX, tempTile.tileY)) {
                            tempTile.number = -1;
                        }
                        
                        if (tempTile.number >= -1) {
                            tempQueue.push(tempTile);
                        }
                    }

                    // Restore PQ
                    pq = tempQueue;

            } else {

                // If the PQ update is already performed in the last action
                // Need to use model checking with frontiers (Propositional Logic) 
                // At the moment, PQ contains only frontier tiles                  

                // Clear passedAssignments and coveredFrontier vector for the new round of model checking
                passedAssignments.clear();
                coveredFrontier.clear();

                // A copy of PQ, act as uncoveredFrontier
                std::vector<usingTile> uncoveredFrontierVector;

                /*
                    Get all uncovered frontier.
                    If this is the first time using the frontier, coveredFrontier starts out empty.
                    If this is not the first time, more frontier tiles will be added into coveredFrontier.
                    coveredFrontier's size will exceed 10 tiles, but not by much, to minimize computation time.
                */
                while (pq.empty() != true && coveredFrontier.size() < 10) {
                    usingTile temp = pq.top();

                    std::queue<std::pair<int, int>> coveredNeighborList = getAllCoveredNeighbors(temp.tileX, temp.tileY);
                    while (coveredNeighborList.empty() != true) {

                        // If the covered tile is not already in coveredFrontier, push it into the vector.
                        if (findInVector(coveredFrontier, coveredNeighborList.front()) == -1) {
                            coveredFrontier.push_back(coveredNeighborList.front()); 
                        }
                        coveredNeighborList.pop();

                    }
                    pq.pop();
                    uncoveredFrontierVector.push_back(temp);

                }


                // Restore PQ
                for (usingTile t : uncoveredFrontierVector) {
                    pq.push(t);
                }
                    
                int coverFrontierSize = coveredFrontier.size();
                int uncoveredFrontierVectorSize = uncoveredFrontierVector.size();
                    

                /*
                    Simulating each possible situation the covered frontier can be.
                    Each column of truthTableRow represent each tile in coveredFrontier.
                    Index of truthTable = Index of coveredFrontier
                    Assign 0 or 1 to each column to indicate safe or mine.
                */
                int i = 1;
                truthTableRow = 0;

                /*  
                    1. The do-while loop is responsible for incrementing truthTableRow by 1 to generate all possible
                       safe tiles and mine tiles placement in coveredFrontier
                    2. The do-while loop runs 2^(size of coveredFrontier) times to simulate all possibe assignment of all
                       tiles in coveredFrontier
                */
                do {

                    // numConstraintPassed is the number of uncoveredFrontier tiles that are satisfied by the assignment.
                    
                    int numConstraintPassed = 0;

                    // Check against each EF in frontier sets (constraints). If pass, record.
                    /*
                        For each uncovered frontier:
                            Find all covered neighbor (which is in the 10 covered frontier list)
                            For each covered neighbor:
                                Look up the safe tiles and mine tiles assignment for the current truthTableRow.
                                Sum it up.
                            Check if the sum of all mines in neighbors = # of covered neighbor - effectiveLabel.
                    */
                    for (usingTile t : uncoveredFrontierVector) {

                        int sum = 0;
                        // Find all the covered neighbor of the current uncovered frontier tile
                        std::queue<std::pair<int, int>> adj = getAllCoveredNeighbors(t.tileX, t.tileY);
                            
                        // 
                        while (adj.empty() != true) {
                            int index = findInVector(coveredFrontier, adj.front());
                            if (index != -1) {
                                // Sum from bitset
                                sum += truthTableRow[index];
                            }
                            adj.pop();
                        }
                            
                        // If sum of the 1's from neighboring covered tiles == the mine left, pass 1 constraint.
                        if (sum == 0 && effectiveLabels[t.tileX][t.tileY] == -1) {
                            numConstraintPassed++;
                        } else if (sum == getNumNeighborCovered(t.tileX, t.tileY) - effectiveLabels[t.tileX][t.tileY]) {
                            numConstraintPassed++;
                        }
                    }

                    // If the current assignment corresponds with effectiveLabels of every tiles in uncovered frontier,
                    // then it is a solution.
                    if (numConstraintPassed == uncoveredFrontierVectorSize) {
                        passedAssignments.push_back(truthTableRow);
                    }

                    unsigned long l = truthTableRow.to_ulong() + 1;
                    truthTableRow = (int)(l);
                    i++;


                } while (i <= std::pow(2, coverFrontierSize));

                    

                if (passedAssignments.size() == 1) {
                    
                    for (int i = 0; i < coveredFrontier.size(); i++) {
                            
                        if (passedAssignments[0][i] == 0) {
                            safeCoveredFrontierTile.push(coveredFrontier[i]);
                        } else if (passedAssignments[0][i] == 1) {
                            mineCoveredFrontierTile.push(coveredFrontier[i]);
                        }
                    }


                    if (mineCoveredFrontierTile.size() > 0) {

                        std::pair<int, int> current = mineCoveredFrontierTile.front();
                        mineCoveredFrontierTile.pop();
                        agentX = current.first;
                        agentY = current.second;
                        flagCount++;
                        coveredFrontier.erase(coveredFrontier.begin() + findInVector(coveredFrontier, current));
                        pqUpdate = false;
                        effectiveLabels[current.first][current.second] = -3;
                        return {FLAG, current.first - 1, current.second - 1};

                    } else {

                        std::pair<int, int> current = safeCoveredFrontierTile.front();
                        safeCoveredFrontierTile.pop();
                        agentX = current.first;
                        agentY = current.second;
                        uncoverCount++;
                        updateEffectiveLabels(current.first, current.second);
                        coveredFrontier.erase(coveredFrontier.begin() + findInVector(coveredFrontier, current));
                        pqUpdate = false;
                        return {UNCOVER, current.first - 1, current.second - 1};
                    }

                } else {

                    // More than 1 successful assignment or 0 successful assignment
                    int successSize = passedAssignments.size();
                    std::priority_queue<tileProb, vector<tileProb>, compareMaxProb> maxProb;

                    if (successSize > 0) {

                            // Get all covered tile in frontier that is not a mine --> safeCoveredFrontierTile
                        /*
                            Compare the index i of each successful assignments to the index i of the first successful assignment
                            Break as soon as a mismatch is found.
                            If there is a consistent index in all successful assignments, add to:
                                mineCoveredFrontierTile if 1
                                safeCoveredFrontierTile if 0
                            If no consistency is found, mineCoveredFrontierTile and mineCoveredFrontierTile have size 0.
                        */
                        for (int bitIndex = 0; bitIndex < coveredFrontier.size(); bitIndex++) {
                            bool sameBit = true;
                            int bit = passedAssignments[0][bitIndex];

                            for (int assignmentIndex = 1; assignmentIndex < successSize; assignmentIndex++) {
                                if (bit != passedAssignments[assignmentIndex][bitIndex]) {
                                    sameBit = false;
                                    break;
                                }
                            }

                            if (sameBit == true && bit == 0) {
                                std::pair<int, int> safeTile = coveredFrontier[bitIndex];
                                if (existInQueue(safeCoveredFrontierTile, safeTile) != true) {
                                    safeCoveredFrontierTile.push(safeTile);
                                }
                            } else if (sameBit == true && bit == 1) {
                                std::pair<int, int> nsafeTile = coveredFrontier[bitIndex];
                                if (existInQueue(mineCoveredFrontierTile, nsafeTile) != true) {
                                    mineCoveredFrontierTile.push(nsafeTile);
                                }
                            }

                        }

 
                        if (mineCoveredFrontierTile.size() > 0) {

                            std::pair<int, int> current = mineCoveredFrontierTile.front();
                            mineCoveredFrontierTile.pop();
                            agentX = current.first;
                            agentY = current.second;
                            flagCount++;
                            coveredFrontier.erase(coveredFrontier.begin() + findInVector(coveredFrontier, current));
                            pqUpdate = false;
                            effectiveLabels[current.first][current.second] = -3;
                            return {FLAG, current.first - 1, current.second - 1};
                        }

                        if (safeCoveredFrontierTile.size() == 0) {

                            /*
                                For each tiles in coveredFrontier:
                                    Count how many of the successful assignments the tile is a mine (1)
                                Compute the probability of the tile being a mine --> P(mine) = assignment count / total successful assignment

                            */
                            for (int bitIndex = 0; bitIndex < coveredFrontier.size(); bitIndex++) {
                                int countMineProb = 0;

                                for (int assignmentIndex = 0; assignmentIndex < passedAssignments.size(); assignmentIndex++) {
                                    if (passedAssignments[assignmentIndex][bitIndex] == 1) {
                                        countMineProb++;
                                    }
                                }

                                maxProb.push(tileProb{coveredFrontier[bitIndex].first, coveredFrontier[bitIndex].second, (double)countMineProb / (double)(passedAssignments.size())});
                                    
                            }


                            tileProb current = maxProb.top();
                            maxProb.pop();
                            agentX = current.tileX;
                            agentY = current.tileY;
                            flagCount++;
                            std::pair<int, int> p {current.tileX, current.tileY};
                            coveredFrontier.erase(coveredFrontier.begin() + findInVector(coveredFrontier, p));
                            pqUpdate = false;
                            effectiveLabels[current.tileX][current.tileY] = -3;
                            return {FLAG, current.tileX - 1, current.tileY - 1};

                                
                        } else {

                            // There is no sure mine tile, but there is a sure safe tile!
                            std::pair<int, int> notMine = safeCoveredFrontierTile.front();
                            safeCoveredFrontierTile.pop();
                            agentX = notMine.first;
                            agentY = notMine.second;
                            uncoverCount++;
                            updateEffectiveLabels(notMine.first, notMine.second);
                            pqUpdate = false;
                            coveredFrontier.erase(coveredFrontier.begin() + findInVector(coveredFrontier, notMine));
                            return {UNCOVER, notMine.first - 1, notMine.second - 1};
                    
                        }

                    } else {

                        std::pair<int, int> guessTile = getRandomCoveredFrontierTile();
                        agentX = guessTile.first;
                        agentY = guessTile.second;
                        uncoverCount++;
                        updateEffectiveLabels(guessTile.first, guessTile.second);
                        coveredFrontier.erase(coveredFrontier.begin() + findInVector(coveredFrontier, guessTile));
                        pqUpdate = false;
                        return {UNCOVER, guessTile.first - 1, guessTile.second - 1};      

                    }

                }
                   
            }

        }
    }

}


int MyAI::getNumNeighborCovered(int x, int y) {
    

    int counter = 0;

    for (unsigned int i = x - 1; i <= x + 1; i++) {
        if (i <= colDimension && i > 0 && y != 1) 
        {
            if (effectiveLabels[i][y - 1] == -2) {
                counter++;
            }
        }
        if (i <= colDimension && i > 0 && y < rowDimension)
        {
            if (effectiveLabels[i][y + 1] == -2) {
                counter++;
            }
        }
    }
    if (x <= colDimension && x > 1 && effectiveLabels[x - 1][y] == -2) {
        counter++;
    }
    if (x < colDimension && x >= 1 && effectiveLabels[x + 1][y] == -2) {
        counter++;
    }

    return counter;

}


std::pair<int, int> MyAI::getCoveredNeighbor(int x, int y) {

    std::pair<int, int> safeNeighbor;
    safeNeighbor.first = 0;
    safeNeighbor.second = 0;


    if (x - 1 >= 1 && y - 1 >= 1 && effectiveLabels[x - 1][y - 1] == -2) {
        safeNeighbor.first = x - 1;
        safeNeighbor.second = y - 1;
        
    } else if (y - 1 >= 1 && effectiveLabels[x][y - 1] == -2) {
        safeNeighbor.first = x;
        safeNeighbor.second = y - 1;
        
    } else if (x + 1 <= colDimension && y - 1 >= 1 && effectiveLabels[x + 1][y - 1] == -2) {
        safeNeighbor.first = x + 1;
        safeNeighbor.second = y - 1;

    } else if (x - 1 >= 1 && y + 1 <= rowDimension && effectiveLabels[x - 1][y + 1] == -2) {
        safeNeighbor.first = x - 1;
        safeNeighbor.second = y + 1;

    } else if (y + 1 <= rowDimension && effectiveLabels[x][y + 1] == -2) {
        safeNeighbor.first = x;
        safeNeighbor.second = y + 1;

    } else if (x + 1 <= colDimension && y + 1 <= rowDimension && effectiveLabels[x + 1][y + 1] == -2) {
        safeNeighbor.first = x + 1;
        safeNeighbor.second = y + 1;

    } else if (x - 1 >= 1 && effectiveLabels[x - 1][y] == -2) {
        safeNeighbor.first = x - 1;
        safeNeighbor.second = y;

    } else if (x + 1 <= colDimension && effectiveLabels[x + 1][y] == -2) {
        safeNeighbor.first = x + 1;
        safeNeighbor.second = y;
    }

    return safeNeighbor;

}


int MyAI::getNumFlagNeighbor(int x, int y) {

    int counter = 0;

    for (unsigned int i = x - 1; i <= x + 1; i++) {
        if (i <= colDimension && i > 0 && y != 1) 
        {
            if (effectiveLabels[i][y - 1] == -3) {
                counter++;
            }
        }
        if (i <= colDimension && i > 0 && y < rowDimension)
        {
            if (effectiveLabels[i][y + 1] == -3) {
                counter++;
            }
        }
    }
    if (x <= colDimension && x > 1 && effectiveLabels[x - 1][y] == -3) {
        counter++;
    }
    if (x < colDimension && x >= 1 && effectiveLabels[x + 1][y] == -3) {
        counter++;
    }

    return counter;

}

void MyAI::updateEffectiveLabels(int x, int y) {

    if (x - 1 >= 1 && y - 1 >= 1 && effectiveLabels[x - 1][y - 1] > 0) {
        effectiveLabels[x - 1][y - 1]--;
    }
        
    if (y - 1 >= 1 && effectiveLabels[x][y - 1] > 0) {
        effectiveLabels[x][y - 1]--;
    }
        
    if (x + 1 <= colDimension && y - 1 >= 1 && effectiveLabels[x + 1][y - 1] > 0) {
        effectiveLabels[x + 1][y - 1]--;
    }

    if (x - 1 >= 1 && y + 1 <= rowDimension && effectiveLabels[x - 1][y + 1] > 0) {
        effectiveLabels[x - 1][y + 1]--;
    }

    if (y + 1 <= rowDimension && effectiveLabels[x][y + 1] > 0) {
        effectiveLabels[x][y + 1]--;
    }

    if (x + 1 <= colDimension && y + 1 <= rowDimension && effectiveLabels[x + 1][y + 1] > 0) {
        effectiveLabels[x + 1][y + 1]--;
    }

    if (x - 1 >= 1 && effectiveLabels[x - 1][y] > 0) {
        effectiveLabels[x - 1][y]--;
    }
    
    if (x + 1 <= colDimension && effectiveLabels[x + 1][y] > 0) {
        effectiveLabels[x + 1][y]--;
    }

}


void MyAI::printPQ(std::priority_queue<usingTile, vector<usingTile>, compareNumber> pq) {

    while (pq.empty() != true) {
        usingTile tempE = pq.top();
        std::cout << tempE.tileX << " " << tempE.tileY << " " << tempE.number << "\t";
        pq.pop();
    }
    std::cout << std::endl;
}


void MyAI::printMinProb(std::priority_queue<tileProb, vector<tileProb>, compareProb> tb) {
    while (tb.empty() != true) {
        tileProb tempE = tb.top();
        std::cout << tempE.tileX << " " << tempE.tileY << " " << tempE.probability << std::endl;
        tb.pop();
    }
}


void MyAI::printQ(std::queue<std::pair<int, int>> q) {

    while (q.empty() != true) {
        std::pair<int, int> tempE = q.front();
        std::cout << tempE.first << " " << tempE.second << "\t";
        q.pop();
    }
    std::cout << std::endl;
}


void MyAI::printVector(std::vector<std::pair<int, int>> v) {
    for (int i = 0; i < v.size(); i++) {
        std::cout << "[" << i << "]: " << v[i].first << " " << v[i].second << "\t";
    }
    std::cout << std::endl;
}


void MyAI::printVector(std::vector<usingTile> u) {
    for (int i = 0; i < u.size(); i++) {
            std::cout << "[" << i << "]: " << u[i].tileX << " " << u[i].tileY << "\t";
    }
    std::cout << std::endl;
}


void MyAI::printPassedAssignments(std::vector<std::bitset<20>> pa) {
    for (int i = 0; i < pa.size(); i++) {
            std::cout << "[" << i << "]: " << pa[i] << std::endl;
    }
}


void MyAI::printEF() {

    for (std::vector<int> x : effectiveLabels) {
        for (int effL : x) {
            std::cout << effL << "\t";
        }
        std::cout << std::endl;
    }

}


std::queue<std::pair<int, int>> MyAI::getAllCoveredNeighbors(int x, int y) {

    std::queue<std::pair<int, int>> result;

    if (x - 1 >= 1 && y - 1 >= 1 && effectiveLabels[x - 1][y - 1] == -2) {
        std::pair<int, int> coveredNeighbor {x - 1, y - 1};
        result.push(coveredNeighbor);
        
    }
    
    if (y - 1 >= 1 && effectiveLabels[x][y - 1] == -2) {
        std::pair<int, int> coveredNeighbor {x, y - 1};
        result.push(coveredNeighbor);
        
    }
    
    if (x + 1 <= colDimension && y - 1 >= 1 && effectiveLabels[x + 1][y - 1] == -2) {
        std::pair<int, int> coveredNeighbor {x + 1, y - 1};
        result.push(coveredNeighbor);

    }
    
    if (x - 1 >= 1 && y + 1 <= rowDimension && effectiveLabels[x - 1][y + 1] == -2) {
        std::pair<int, int> coveredNeighbor {x - 1, y + 1};
        result.push(coveredNeighbor);

    }
    
    if (y + 1 <= rowDimension && effectiveLabels[x][y + 1] == -2) {
        std::pair<int, int> coveredNeighbor {x, y + 1};
        result.push(coveredNeighbor);

    }
    
    if (x + 1 <= colDimension && y + 1 <= rowDimension && effectiveLabels[x + 1][y + 1] == -2) {
        std::pair<int, int> coveredNeighbor {x + 1, y + 1};
        result.push(coveredNeighbor);

    }
    
    if (x - 1 >= 1 && effectiveLabels[x - 1][y] == -2) {
        std::pair<int, int> coveredNeighbor {x - 1, y};
        result.push(coveredNeighbor);

    }
    
    if (x + 1 <= colDimension && effectiveLabels[x + 1][y] == -2) {
        std::pair<int, int> coveredNeighbor {x + 1, y};
        result.push(coveredNeighbor);
    }

    return result;

}


std::set<std::pair<int, int>> MyAI::getAllUncoveredFrontiers(int x, int y) {

    std::set<std::pair<int, int>> result;

    if (x - 1 >= 1 && y - 1 >= 1 && effectiveLabels[x - 1][y - 1] >= 1) {
        std::pair<int, int> coveredNeighbor {x - 1, y - 1};
        result.insert(coveredNeighbor);
        
    }
    
    if (y - 1 >= 1 && effectiveLabels[x][y - 1] >= 1) {
        std::pair<int, int> coveredNeighbor {x, y - 1};
        result.insert(coveredNeighbor);
        
    }
    
    if (x + 1 <= colDimension && y - 1 >= 1 && effectiveLabels[x + 1][y - 1] >= 1) {
        std::pair<int, int> coveredNeighbor {x + 1, y - 1};
        result.insert(coveredNeighbor);

    }
    
    if (x - 1 >= 1 && y + 1 <= rowDimension && effectiveLabels[x - 1][y + 1] >= 1) {
        std::pair<int, int> coveredNeighbor {x - 1, y + 1};
        result.insert(coveredNeighbor);

    }
    
    if (y + 1 <= rowDimension && effectiveLabels[x][y + 1] >= 1) {
        std::pair<int, int> coveredNeighbor {x, y + 1};
        result.insert(coveredNeighbor);

    }
    
    if (x + 1 <= colDimension && y + 1 <= rowDimension && effectiveLabels[x + 1][y + 1] >= 1) {
        std::pair<int, int> coveredNeighbor {x + 1, y + 1};
        result.insert(coveredNeighbor);

    }
    
    if (x - 1 >= 1 && effectiveLabels[x - 1][y] >= 1) {
        std::pair<int, int> coveredNeighbor {x - 1, y};
        result.insert(coveredNeighbor);

    } 
    
    if (x + 1 <= colDimension && effectiveLabels[x + 1][y] >= 1) {
        std::pair<int, int> coveredNeighbor {x + 1, y};
        result.insert(coveredNeighbor);
    }

    return result;

}


std::vector<std::pair<int, int>> MyAI::getBoardCoveredTiles() {

    std::vector<std::pair<int, int>> result;

    for (int i = 1; i <= colDimension; i++) {
        for (int j = 1; j <= rowDimension; j++) {
            if (effectiveLabels[i][j] == -2) {
                std::pair<int, int> p{i, j};
                result.push_back(p);
            }
        }
    }

    return result;

}


std::pair<int, int> MyAI::getRandomCoveredTile() {
    std::vector<std::pair<int, int>> v = getBoardCoveredTiles();
    int index = std::rand() % v.size();
    return v[index];
}


std::pair<int, int> MyAI::getRandomCoveredFrontierTile() {
    int index = std::rand() % coveredFrontier.size();
    return coveredFrontier[index];
}


int MyAI::findInVector(std::vector<std::pair<int, int>> v, std::pair<int, int> p) {

    int index = 0;

    for (std::pair<int, int> elem : v) {
        if (elem == p) {
            return index;
        }
        index++;
    }

    return -1;

}


bool MyAI::existInQueue(std::queue<std::pair<int, int>> q, std::pair<int, int> p) {

    while (q.empty() != true) {
        if (q.front() == p) {
            return true;
        }
        q.pop();
    }

    return false;

}
