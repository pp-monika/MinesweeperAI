// ======================================================================
// FILE:        World.cpp
//
// AUTHOR:      Jian Li
//
// DESCRIPTION: This file contains the world class, which is responsible
//              for everything game related.
//
// NOTES:       - Don't make changes to this file.
// ======================================================================
//

#include "World.hpp"

using namespace std;

// ===============================================================
// =				Constructor and Destructor
// ===============================================================

World::World(bool _debug, string aiType, string filename)
{
    // Operation Flags
    debug = _debug;

    // World Initialization
    // True for file provided; false for file not provided, board with default size and random feature
    if ( !filename.empty() )
    {

        // A text file for constructing the board is given.

        // open file
        ifstream file;
        file.open(filename);

        file >> rowDimension >> colDimension;
        // std::cout << "file rowD = " << rowDimension << std::endl;
        // std::cout << "file colD = " << colDimension << std::endl;

        if (file.fail())
            throw exception();

        // Board is a 2-D array of Tiles in the form [col][row]
        board = new Tile*[colDimension];
        for ( int index = 0; index < colDimension; ++index )
            board[index] = new Tile[rowDimension];

        // The 2 digits on the second row corresponds to the first safe tile that will be uncovered
        // at the beginning of the game.
        file >> agentX >> agentY;
        // std::cout << "file agentX = " << agentX << std::endl;
        // std::cout << "file agentY = " << agentY << std::endl;

        // UNCOVER the first tile at position [agentX - 1, agentY - 1] <-- index start at 0
        lastAction = genFirstAxis(--agentX, --agentY);
        // Assigning mine value to each Tile according to given file and updating neighbor mine count for each Tile
        addFeatures ( file );
        file.close();

    }
    else
    {

        // No file is given. Generate default board.
        totalMines      = 10;
        colDimension    = 8;
        rowDimension    = 8;
        board = new Tile*[colDimension];
        for ( int index = 0; index < colDimension; ++index )
            board[index] = new Tile[rowDimension];

        lastAction   = genFirstAxis();
        agentX       = lastAction.x;
        agentY       = lastAction.y;

        addFeatures();
    }

    maxMoves = rowDimension * colDimension * 2;

    // For scoring purpose
    switch (colDimension)
    {
        case 8:
            Bonus = 1;
            break;
        case 16:
            Bonus = 2;
            break;
        case 30:
            Bonus = 3;
            break;
        default:
            Bonus = 1;
            break;
    }

    // Agent Initialization
    score      = 0;
    coveredTiles = rowDimension * colDimension - 1; // Exclude first UNCOVERED Tile
    flagLeft   = totalMines;

    if (aiType == "randomAI")
        agent = new RandomAI( rowDimension, colDimension, totalMines, agentX, agentY );

    else if (aiType == "manualAI")
        agent = new ManualAI( rowDimension, colDimension, totalMines, agentX, agentY );

    else
        agent = new MyAI( rowDimension, colDimension, totalMines, agentX, agentY );

}

World::~World() {
    for ( int index = 0; index < colDimension; ++index )
        delete [] board[index];

    delete [] board;
}

// ===============================================================
// =					Engine Function
// ===============================================================

int World::run()
{
    int perceptNumber;
    bool gameOver = false;
    int move = 0;

    // WHile the game is not over and there are moves left, keep going
    while ( !gameOver && move < maxMoves )
    {
        if ( debug || dynamic_cast<ManualAI*>(agent))
        {
            printWorldInfo();

            if ( !dynamic_cast<ManualAI*>(agent) )
            {
                // Pause the game, only if manualAI isn't on
                // because manualAI pauses for us
                cout << "Press ENTER to continue..." << endl;
                cin.ignore( 999, '\n');
            }
        }

        // If most recent action is UNCOVER, Agent now knows # of neighbor mines
        if (lastAction.action == Agent::UNCOVER)
            perceptNumber = board[agentX][agentY].number;
        else
            perceptNumber = -1;

        /*
        -- Where we make use of AI
        -- Given # of neighboring mines of the most recent UNCOVERED Tile,
        -- what action should Agent do next?
        -- getAction() takes in # of mines in neighborhood and return action that Agent should take
        */
        lastAction = agent->getAction( perceptNumber );

        // Make the move
        // Make change to the Tile according to the action Agent does
        gameOver = doMove();

        move++;
    }

    return score;
}


// ===============================================================
// =				World Generation Functions
// ===============================================================
void World::addFeatures(    )
// Adding mines, adding mine counter according to neighbour, uncover first file
{
    addMine();
    // Generate number of mines around
    addMineCount();
}

void World::addFeatures( std::ifstream &file )
// set feature according to the file
{

    int r = rowDimension;
    bool mine = 0;

    // generate mine according to input file
    while (r > 0 &&!file.eof() )
    {
        --r;

        for ( int c = 0; c < colDimension; ++c )
        {
            file >> mine;

            if (file.fail())
                throw exception();
            if (mine)
            {
                board[c][r].mine = mine;
                ++totalMines;
            }
        }
    }

    addMineCount();
}

Agent::Action World::genFirstAxis(  )
// Generate random first move axis: in bound, has no mine, no neighbour has mine
// return agent for first move coordinates info
{
    int fc = randomInt( colDimension );
    int fr = randomInt( rowDimension );
    while ( !isInBounds( fc, fr ))
    {
        fc = randomInt( colDimension );
        fr = randomInt( rowDimension );
    }
    board[fc][fr].uncovered = true;

    std::cout << "First X = " << (int) fc << std::endl;
    std::cout << "First Y = " << (int) fr << std::endl;

    return {Agent::UNCOVER, (int) fc , (int )fr};
}

Agent::Action World::genFirstAxis(int c, int r) {

    try{
        if (!isInBounds(c, r) || board[c][r].mine || board[c][r].number) {
            std::cout << "first move x = " << c << std::endl;
            std::cout << "first move y = " << r << std::endl;
            throw "[ERROR] First move coordinates are invalid.";
        }
    }catch (const char* msg){
        cerr << msg << endl;
        exit(0);
    }

    // change status of Tile at [c][r] to UNCOVERED
    board[c][r].uncovered = true;
    return {Agent::UNCOVER, c, r};
}


void World::addMine(    )
// Generate mine: totalMines times, in bound, no mine before -> [mc][mr]mine = true,
// not adding mine around and on the first move uncover tile
{
    for (int m = 0; m < totalMines; ++m){
        int mc = randomInt( colDimension );
        int mr = randomInt( rowDimension );
        while ( !isInBounds( mc, mr ) || board[mc][mr].mine || ((agentX - 2 < mc && mc < agentX + 2) && (agentY - 2 < mr && mr < agentY + 2)) )
        {
            mc = randomInt( colDimension );
            mr = randomInt( rowDimension );
        }
        board[mc][mr].mine = true;
    }
}

void World::addMineCount(   )
// Generate number of mines around
// Assign neighbor mine count to each Tile in the board
{
    for ( int c = 0; c < colDimension; ++c ){
        for ( int r = 0; r < rowDimension; ++r ){
            if (!board[c][r].mine)
                addNeighbour( c, r );
        }
    }
}

void World::addNeighbour( int c, int r)
{
    // helper function for addMineCount
    // iterate 8 neighbours around a tile, and increment neighbour if there is mine

    int dir[8][2] = { {-1, 1}, {-1, 0}, {-1 , -1},
                      {0, 1},          {0, -1},
                      {1, 1}, {1, 0}, {1, -1} };

    for (int *i : dir) {
        int nc = c + i[0];
        int nr = r + i[1];
        if ( isInBounds( nc, nr ) && board[nc][nr].mine ){
            board[c][r].number++;
        }
    }
}

void World::uncoverAll()
{

    for ( int c = 0; c < colDimension; ++c )
    {
        for ( int r = 0; r < rowDimension; ++r )
            board[c][r].uncovered = true;
    }
    if ( debug || dynamic_cast<ManualAI*>(agent) )
        printWorldInfo();
}

// Make change to the Tile's attributes according to the action Agent does
bool World::doMove()
{
    agentX       = lastAction.x;
    agentY       = lastAction.y;

    switch ( lastAction.action )
    {
        case Agent::LEAVE:
            if (coveredTiles == totalMines)
                score += Bonus;
            uncoverAll();
            return true;
        case Agent::UNCOVER:
            if (board[agentX][agentY].mine)
            {
                uncoverAll();
                return true;
            }

            else if (!board[agentX][agentY].uncovered)
            {
                board[agentX][agentY].uncovered = true;
                --coveredTiles;
            }

            break;
        case Agent::FLAG:
            if (flagLeft)
            {
                board[agentX][agentY].flag = true;
                --flagLeft;
                if (board[agentX][agentY].mine)
                    ++correctFlags;
                else
                    --correctFlags;
            }
            break;
        case Agent::UNFLAG:
            if (board[agentX][agentY].flag)
            {
                board[agentX][agentY].flag = false;
                ++flagLeft;
                if (board[agentX][agentY].mine)
                    --correctFlags;
                else
                    ++correctFlags;
            }
            break;
    }

    return false;
}

bool World::isInBounds ( int c, int r )
{
    return ( 0 <= c && c < colDimension && 0 <= r && r < rowDimension );
}

// ===============================================================
// =				World Printing Functions
// ===============================================================

void World::printWorldInfo(     )
{
    printBoardInfo();
    printAgentInfo();
}

void World::printBoardInfo(     )
{
    cout << "---------------- Game Board ------------------\n" << endl;
    for ( int r = rowDimension-1; r >= 0; --r )
    {
        printf("%-4d%c",r+1,'|');
        for ( int c = 0; c < colDimension; ++c )
            printTileInfo ( c, r );
        cout << endl << endl;
    }

    cout << "     ";
    for (int c = 0; c < colDimension; ++c)
        cout << setw(8) << "-" ;
    cout << endl << "     ";
    for (int c = 0; c < colDimension; ++c)
        cout << setw(8) << c + 1;
    cout << endl;
}

void World::printTileInfo( int c, int r )
{

    string tileString;

    if ( board[c][r].uncovered )
        if ( board[c][r].mine )
            tileString.append("*");
        else
        {
            tileString.append(to_string(board[c][r].number));

        }
    else if ( board[c][r].flag )
            tileString.append("#");
    else
        tileString.append(".");

    cout << setw(8) << tileString;
}

void World::printAgentInfo()
{
    cout << "\n------------------ Percepts ------------------ " << endl;
    cout << "Tiles Covered: " << coveredTiles;
    cout << " Flags Left: " << flagLeft << "    ";


    printActionInfo ();
}

void World::printActionInfo()
{
    switch ( lastAction.action )
    {
        case Agent::UNCOVER:
            cout << "Last Action: Uncover";
            break;
        case Agent::FLAG:
            cout << "Last Action: Flag";
            break;
        case Agent::UNFLAG:
            cout << "Last Action: Unflag";
            break;
        case Agent::LEAVE:
            cout << "Last Action: Leave" << endl;
            break;

        default:
            cout << "Last Action: Invalid" << endl;
    }

    if (lastAction.action != Agent::LEAVE)
        cout << " on tile " << agentX + 1 << " " << agentY + 1 << endl;
}

// ===============================================================
// =					Helper Functions
// ===============================================================

int World::randomInt ( int limit )
{
    return rand() % limit;
}









