// Project Identifier: 40FB54C86566B9DDEAB902CC80E8CE85C1C62AAD
// EECS281, Project 1

#include <stdio.h>
#include <vector>
#include <stack>
#include <queue>
#include <deque>
#include <getopt.h>
#include <iostream>
#include <string>
#include "xcode_redirect.hpp"

using namespace std;

struct MapGrid {
    size_t row;
    size_t col;
    char terrain;
    char fromDirection; //direction of discovery
    bool discovered;
};

// global variables
size_t mapSize = 0;
string sizeString;
size_t startRow = 0;
size_t startCol = 0;
size_t treasureRow = 0;
size_t treasureCol = 0;
int discoveredWaterCount = 0;
int discoveredLandCount = 0;
int wentAshoreCount = 0;
size_t pathSize = 0;

// COMMAND LINE OPTIONS
bool cStack = true;
bool cQueue = !cStack;
bool fStack = false;
bool fQueue = !fStack;
string huntOrder = "nesw";
bool verboseMode = false;
string inputMode;
bool showPath = false;
string showPathMode = "M";
bool showStats = false;

/**
 * Print verbose info
 *
 */
void print_verbose(const string& s) {
    if (verboseMode) {
        cout << s << "\n";
    }
}

/**
 * Count number of apprances of a charactor in a string.
 *
 */
size_t count_char(const string& s, char c) {
    size_t retval = 0;
    size_t len = s.length(); //had to change to size_t
    
    for (size_t i = 0; i < len; i++) {
        if (s[i] == c) {
            retval++;
        }
    }
    return retval;
}

/**
 * Check whether the hunter is valid or not.
 * The order can be a string using exactly once of these four chars: n, e, w, s
 *
 */
bool check_hunt_order(const string& order) {
    
    bool retval = ((order.length() == 4) &&
                   (count_char(order, 'n') == 1) &&
                   (count_char(order, 'e') == 1) &&
                   (count_char(order, 's') == 1) &&
                   (count_char(order, 'w') == 1));
    
    return retval;
}

void get_options(int argc, char** argv) {
    
    int option_index = 0, option = 0;
    opterr = false;
    bool gotShowPath = false;
    
    struct option longOpts[] = {
        {"help", no_argument, nullptr, 'h'},
        {"captain", required_argument, nullptr, 'c'},
        {"first-mate", required_argument, nullptr, 'f'},
        {"hunt-order", required_argument, nullptr, 'o'},
        {"verbose", no_argument, nullptr, 'v'},
        {"stats", no_argument, nullptr, 's'},
        {"show-path", required_argument, nullptr, 'p'},
        { nullptr, 0, nullptr, '\0' }
    };
    
    while ((option = getopt_long(argc, argv, "hc:f:o:vsp:", longOpts,
                                 &option_index)) != -1) {
        switch (option) {
            case 'h':
                cout << "This program finds a path from the starting\n"
                << "location to the buried treasure and prints the\n"
                << "details of the hunt.\n"
                <<  "Usage: \'./hunt\n\t[--help | -h]\n"
                <<                      "\t[--captain | -c] <\"queue\"|\"stack\">\n"
                <<                      "\t[--first-mate | -f] <\"queue\"|\"stack\">\n"
                <<                      "\t[--hunt-order | -o]\n"
                <<                      "\t[--verbose | -v]\n"
                <<                      "\t[--stats | -s]\n"
                <<                      "\t[--show-path | -p] <M|L>\n"
                <<                      "\t< inputfile>\n";
                exit(0);
                
            case 'c':
                if (strcmp(optarg, "stack") == 0) {
                    cStack = true;
                    cQueue = false;
                }
                else if (strcmp(optarg, "queue") == 0) {
                    cQueue = true;
                    cStack = false;
                }
                else {
                    exit(1);
                }
                break;
            case 'f':
                if (strcmp(optarg, "stack") == 0) {
                    fStack = true;
                    fQueue = false;
                }
                else if (strcmp(optarg, "queue") == 0) {
                    fQueue = true;
                    fStack = false;
                }
                else {
                    exit(1);
                }
                break;
            case 'o':
                if (optarg) {
                    if (!check_hunt_order(optarg)) {
                        exit(1);
                    }
                    else {
                        huntOrder = optarg;
                    }
                }
                break;
            case 'v':
                verboseMode = true;
                break;
            case 's':
                showStats = true;
                break;
            case 'p':
                if (gotShowPath) {
                    exit(1);
                }
                showPath = true;
                
                if (strcmp(optarg, "M") == 0) {
                    showPathMode = "M";
                }
                else if (strcmp(optarg, "L") == 0) {
                    showPathMode = "L";
                }
                else {
                    exit(1);
                }
                gotShowPath = true;
                
                break;
            default:
                exit(1);
        }
    }
}

/**
 *  Read the data as map.
 *
 */
void read_map_data(vector<vector<MapGrid> >& map) {
    char c;
    
    for (size_t i = 0; i < mapSize; i++) {
        for (size_t j = 0; j < mapSize; j++) {
            cin >> c;
            
            if (c == '.' || c == 'o' || c == '#' || c == '@' || c == '$') {
                MapGrid square = {i, j, c, 0, false}; //what would the from direction be
                map[i][j] = square;
                
                //if starting point, set starting coordinates
                if (c == '@') {
                    startRow = i;
                    startCol = j;
                }
                else if (c == '$') {
                    treasureRow = i;
                    treasureCol = j;
                }
            }
        }
    }
}

/**
 *  Read the data as List.
 *
 */
void read_list_data(vector<vector<MapGrid> >& map) {
    string ctt;
    size_t row;
    size_t col;
    
    //fill map as all water
    for (size_t i = 0; i < mapSize; i++) {
        for (size_t j = 0; j < mapSize; j++) {
            MapGrid temp = {i, j, '.', 0, false};
            map[i][j] = temp;
        }
    }
    
    // Read the data from list by over writing the terrain
    while(getline(cin, ctt)) {
        if (ctt.empty() || ctt.length() < 5) {
            continue;
        }
        
        size_t cttLen = ctt.length();
        
        string space = " ";
        size_t firstSpaceIndex = ctt.find(space);
        
        string rowStr = ctt.substr(0, firstSpaceIndex);
        row = static_cast<size_t>(stoi(rowStr));
        
        string secondPart = ctt.substr(firstSpaceIndex+1, cttLen);
        size_t secondSpaceIndex = secondPart.find(space);
        
        string colStr = secondPart.substr(0, secondSpaceIndex);
        char terrain = secondPart[secondPart.length()-1];
        
        col = static_cast<size_t>(stoi(colStr));
        
        map[row][col].terrain = terrain;
        MapGrid square = {row, col, map[row][col].terrain, 0, false}; //what is fromDirection
        map[row][col] = square;
        if (map[row][col].terrain == '@') {
            startRow = row;
            startCol = col;
        }
        else if (map[row][col].terrain == '$') {
            treasureRow = row;
            treasureCol = col;
        }
    }
}


/**
 *  Read data from the input file, depending on the mode.
 *
 **/
void read_data(vector<vector<MapGrid> >& map) {
    
    // Skip comment lines
    getline(cin, inputMode); //**program stops**
    while (inputMode.front() == '#' || inputMode.empty()) {
        getline(cin, inputMode);
    }
    
    getline(cin, sizeString); // TODO: check whether it is empty
    mapSize = static_cast<size_t>(stoi(sizeString)); // TODO: check whether it is a valid number
    
    // Set the map size
    map.resize(mapSize, vector<MapGrid>(mapSize));
    
    if (inputMode.compare("M") == 0) { //change
        read_map_data(map);
    }
    else if (inputMode.compare("L") == 0) { //change
        read_list_data(map);
    }
}

/**
 *  Get the next grid with the coordinates based on the hunt order
 *
 */
bool get_next_grid_rowcol(const size_t& currentRow,
                          const size_t& currentCol,
                          const char& c,
                          size_t& nextGrid_row,
                          size_t& nextGrid_col) {
    bool retval = true;
    
    // Depending on the fromDirection c, adjust the row and col indexes
    if (c == 'n') {
        nextGrid_col = currentCol;
        if (currentRow == 0) {
            retval = false;
        }
        else {
            nextGrid_row = currentRow - 1;
        }
    } else if (c == 'w') {
        if (currentCol == 0) {
            retval = false;
        }
        else {
            nextGrid_col = currentCol - 1;
        }
        nextGrid_row = currentRow;
    } else if (c == 'e') {
        if (currentCol + 1 == mapSize) {
            retval = false;
        }
        else {
            nextGrid_col = currentCol + 1;
        }
        nextGrid_row = currentRow;
    } else if (c == 's') {
        nextGrid_col = currentCol;
        if (currentRow + 1 == mapSize) {
            retval = false;
        }
        else {
            nextGrid_row = currentRow + 1;
        }
    }
    return retval;
}

/**
 *  Explore the next grids depending on the hunt order, and who is performing.
 *  This method can be used for both captain and first mate hunt.
 */
bool explore_next_grid(bool isCaptain,
                       const size_t& currentRow,
                       const size_t& currentCol,
                       const char& c,
                       deque<MapGrid>& captainSearch,
                       deque<MapGrid>& firstMateSearch,
                       vector<vector<MapGrid> >& map) {
    bool foundTreasure = false;
    size_t nextGrid_row, nextGrid_col;
    
    bool hasNextGrid = get_next_grid_rowcol(currentRow, currentCol, c,
                                            nextGrid_row, nextGrid_col);
    
    // Only need to explore the grid when there are valid coordinates
    if (hasNextGrid) {
        MapGrid nextGrid = map[nextGrid_row][nextGrid_col];
        
        // only explore the grid if the grid has not been discovered, or impassible
        if (!nextGrid.discovered && !(nextGrid.terrain == '#')) {
            
            // record the from direction and set the discovered to be true
            // record it here even for the treasure grid, for back tracing purpose
            map[nextGrid_row][nextGrid_col].fromDirection = c;
            
            // If treasure is found, no need to continue
            if (nextGrid.terrain == '$') {
                foundTreasure = true;
                
                // If the treasure finding is done by the captain, need to
                // set the landcount
                if (isCaptain) {
                    ++wentAshoreCount;
                    ++discoveredLandCount;
                    string s = "Went ashore at: " + to_string(treasureRow) + "," + to_string(treasureCol);
                    print_verbose(s);
                    string s2 = "Searching island... party found treasure at " + to_string(treasureRow) + "," + to_string(treasureCol) +".";
                    print_verbose(s2);                    
                }
            }
            // Depending on the passible terrain, add the next grid to the
            // proper container depending who is performing the exploration:
            // For a land terrain, we always need to add to the mateSearch queue
            // For water terrain, only captain cares to add to the captain search
            // queue and first mate would ignore it
            else if (nextGrid.terrain == 'o') {
                firstMateSearch.push_back(nextGrid);
                map[nextGrid_row][nextGrid_col].discovered = true;
                //    discoveredLandCount++;
            }
            else if (nextGrid.terrain == '.' && isCaptain) {
                captainSearch.push_back(nextGrid);
                map[nextGrid_row][nextGrid_col].discovered = true;
            }
        }
    }
    return foundTreasure;
}

/**
 *  first mate checks the squares around the current grid,
 *  and add the discovery to the containers accordingly.
 *
 */
bool first_mate_discovery(const size_t& currentRow,
                          const size_t& currentCol,
                          deque<MapGrid>& captainSearch,
                          deque<MapGrid>& firstMateSearch,
                          vector<vector<MapGrid> >& map) {
    bool foundTreasure = false;
    
    // Use hunt order to explore the neigboring grids as first mate
    for (size_t i = 0; i < huntOrder.length(); i++) { // CHANGED TO SIZE_T
        foundTreasure = explore_next_grid(false, currentRow, currentCol,
                                          huntOrder[i],
                                          captainSearch, firstMateSearch, map);
        
        // If treasure is found in the exploration, break out.
        if (foundTreasure) {
            break;
        }
    }
    return foundTreasure;
}

/**
 *  Perform first mate search based on the hunt order
 *
 */
bool perform_mate_search(deque<MapGrid>& captainSearch,
                         deque<MapGrid>& firstMateSearch,
                         vector<vector<MapGrid> >& map) {
    bool foundTreasure = false;
    size_t currentRow = 0;
    size_t currentCol = 0;
    
    // QUEUE OPTION
    if (fQueue) {
        currentRow = firstMateSearch.front().row;
        currentCol = firstMateSearch.front().col;
        firstMateSearch.pop_front();
    }
    // STACK OPTION
    else if (fStack) {
        currentRow = firstMateSearch.back().row;
        currentCol = firstMateSearch.back().col;
        firstMateSearch.pop_back();
    }
    ++discoveredLandCount;
    
    // first mate is going to explore 4 possible directions
    foundTreasure = first_mate_discovery(currentRow, currentCol,
                                         captainSearch, firstMateSearch, map);
    
    if (foundTreasure) {
        ++discoveredLandCount;
        string s3 = "Searching island... party found treasure at " + to_string(treasureRow) + "," + to_string(treasureCol) +".";
        print_verbose(s3);
    }
    return foundTreasure;
}

/**
 *  Depending on the scheme used by first mate search, get the 'first' grid
 *  where the first mate will go ashore.
 */
MapGrid getAshoreGrid(const deque<MapGrid>& firstMateSearch) {
    MapGrid retval;
    
    if (fQueue) {
        retval = firstMateSearch.front();
    }
    else {
        retval = firstMateSearch.back();
    }
    return retval;
}

/**
 *  captain checks the squares around the current grid, and add the discovery to the containers accordingly.
 *
 */
bool captain_discovery(const size_t& currentRow,
                       const size_t& currentCol,
                       deque<MapGrid>& captainSearch,
                       deque<MapGrid>& firstMateSearch,
                       vector<vector<MapGrid> >& map) {
    bool foundTreasure = false;
    bool wentAshore = false;
    
    for (size_t i = 0; i < huntOrder.length(); i++) {
        char c = huntOrder[i];
        foundTreasure = explore_next_grid(true, currentRow, currentCol, c,
                                          captainSearch, firstMateSearch, map);
        
        // If treasure is found in the exploration, break out.
        if (foundTreasure) {
            break;
        }
        // Pause captain search if land is found because firstMateSearch is not empty
        if (!firstMateSearch.empty()) {
            ++wentAshoreCount;
            MapGrid ashoreGrid = getAshoreGrid(firstMateSearch);
            string s = "Went ashore at: " + to_string(ashoreGrid.row) + "," + to_string(ashoreGrid.col);
            print_verbose(s);
        }
        while (!firstMateSearch.empty()) {
            wentAshore = true;
            foundTreasure = perform_mate_search(captainSearch, firstMateSearch, map);
            // If treasure is found, break out the loop
            if (foundTreasure) {
                return foundTreasure;
            }
        }
        // Print out the logs for the first mate search
        if (wentAshore && !foundTreasure) {
            string s = "Searching island... party returned with no treasure.";
            print_verbose(s);
        }
        // reset went ashore
        wentAshore = false;
    }
    
    return foundTreasure;
}

/**
 *  Perform captain search based on the hunter order
 *
 */
bool perform_captain_search(deque<MapGrid>& captainSearch,
                            deque<MapGrid>& firstMateSearch,
                            vector<vector<MapGrid> >& map) {
    bool foundTreasure = false;
    size_t currentRow = 0;
    size_t currentCol = 0;
    
    if (cQueue) {
        currentRow = captainSearch.front().row;
        currentCol = captainSearch.front().col;
        captainSearch.pop_front();
    }
    else if (cStack) {
        currentRow = captainSearch.back().row;
        currentCol = captainSearch.back().col;
        captainSearch.pop_back();
    }
    discoveredWaterCount++;
    
    foundTreasure = captain_discovery(currentRow, currentCol,
                                      captainSearch, firstMateSearch, map);
    
    return foundTreasure;
}

/**
 *  Perform treasure hunt.
 *
 */
bool hunt_treasure(vector<vector<MapGrid> >& map) {
    bool foundTreasure = 0;
    
    // Declear the containers
    deque<MapGrid> captainSearch;
    deque<MapGrid> firstMateSearch;
    
    // start the captain search from the starting square
    captainSearch.push_back(map[startRow][startCol]);
    string s = "Treasure hunt started at: " + to_string(startRow) + "," + to_string(startCol);
    print_verbose(s);
    
    // As long as the captain search container is not empty, keep hunting...
    while (!captainSearch.empty() && !foundTreasure) {
        foundTreasure = perform_captain_search(captainSearch, firstMateSearch, map);
    }
    return foundTreasure;
}

/**
 * Print the hunt logs if the treasure is found.
 *
 */
void get_hunt_path(const vector<vector<MapGrid> >& map, deque<MapGrid>& path) {
    size_t tempRow = treasureRow;
    size_t tempCol = treasureCol;
    
    // Start from the grid where the treasure is. Then use the 'fromDirection' to
    // back trace the grid, until we hit the starting grid.
    while (!(tempRow == startRow && tempCol == startCol)) {
        path.push_back(map[tempRow][tempCol]);
        
        switch (map[tempRow][tempCol].fromDirection) {
            case 'n':
                tempRow = tempRow + 1;
                break;
                
            case 'e':
                tempCol = tempCol - 1;
                break;
                
            case 'w':
                tempCol = tempCol + 1;
                break;
                
            case 's':
                tempRow = tempRow - 1;
                break;
        }
    }
    
    // Finally add the starting grid to the path
    path.push_back(map[startRow][startCol]);
}

/**
 *  Print the hunt path as a list of coordinates.
 *
 */
void print_path_as_list(deque<MapGrid>& path, vector<vector<MapGrid> >& map) {
    deque<MapGrid> landStack;
    deque<MapGrid> waterStack;
    
    // Add  treasure grid to land stack
    landStack.push_back(map[treasureRow][treasureCol]);
    
    // Split the hunt path into two stacks for printing later
    while (!path.empty()) {
        if (path.front().terrain == '.') {
            waterStack.push_back(path.front());
        } else if (path.front().terrain == 'o') {
            landStack.push_back(path.front());
        }
        path.pop_front();
    }
    
    // Add the starting grid to water stack
    waterStack.push_back(map[startRow][startCol]);
    
    cout << "Sail:\n";
    while (!waterStack.empty()) {
        cout << waterStack.back().row << "," << waterStack.back().col << "\n";
        waterStack.pop_back();
    }
    
    cout << "Search:\n";
    while (!landStack.empty()) {
        cout << landStack.back().row << "," << landStack.back().col << "\n";
        landStack.pop_back();
    }
}

/**
 * Print the hunt path as a map.
 *
 */
void print_path_as_map(deque<MapGrid>& path, vector<vector<MapGrid> >& map) {
    
    // Replace the treasure grid with X
    map[treasureRow][treasureCol].terrain = 'X';
    
    // Start from the first grid (treasure grid)
    MapGrid& grid = path.front();
    char prev_from = grid.fromDirection;
    path.pop_front();
    
    // Checking the symbols to put on the path,starting from the treasure grid
    // and go backwards.
    while(path.size() != 1) {
        
        // Get the next grid
        grid = path.front();
        
        // Because the exploration is only 1-directional, only two-consectivie
        // moves have the same direction, the symbol is horizontal or vertical.
        // The rest of them should all be turning symbol.
        if (prev_from == 'n' && grid.fromDirection == 'n') {
            map[grid.row][grid.col].terrain = '|';
        }
        else if (prev_from == 's' && grid.fromDirection == 's') {
            map[grid.row][grid.col].terrain = '|';
        }
        else if (prev_from == 'w' && grid.fromDirection == 'w') {
            map[grid.row][grid.col].terrain = '-';
        }
        else if (prev_from == 'e' && grid.fromDirection == 'e') {
            map[grid.row][grid.col].terrain = '-';
        }
        else {
            map[grid.row][grid.col].terrain = '+';
        }
        
        // Remember the next grid fromDirection
        prev_from = grid.fromDirection;
        
        // Get rid of the next grid
        path.pop_front();
    }
    
    for (size_t i = 0; i < mapSize; i++) {
        for (size_t j = 0; j < mapSize; j++) {
            cout << map[i][j].terrain;
        }
        cout << "\n";
    }
}

/**
 *  Depending on the print option, print the hunt path
 *
 */
void print_hunt_path(deque<MapGrid>& path, vector<vector<MapGrid> >& map) {
    
    if (showPathMode.compare("L") == 0) {
        print_path_as_list(path, map);
    }
    else if (showPathMode.compare("M") == 0) {
        print_path_as_map(path, map);
    }
}

/**
 * Print the stats
 *
 */
void print_stats(bool foundTreasure) {
    cout << "--- STATS ---\n"
    << "Starting location: " << startRow << "," << startCol << "\n"
    << "Water locations investigated: " << discoveredWaterCount << "\n"
    << "Land locations investigated: " << discoveredLandCount << "\n" //landcount different
    << "Went ashore: " << wentAshoreCount << "\n";
    if (foundTreasure) {
        cout << "Path length: " << pathSize << "\n"
        << "Treasure location: " << treasureRow << "," << treasureCol << "\n";
    }
    cout << "--- STATS ---\n";
}

/**
 *  Print hunt logs depending on the command line options.
 *
 */
void print_hunt_logs(bool foundTreasure, vector<vector<MapGrid> >& map) {
    
    if (!foundTreasure) {
        string s = "Treasure hunt failed";
        print_verbose(s);
    }

    // If treasure was found, print the path, and other info
    if (foundTreasure) {
        deque<MapGrid> path;
        get_hunt_path(map, path);
        
        pathSize = path.size() - 1; // not counting the starting grid
        
        if (showStats) {
            print_stats(foundTreasure);
        }
        
        if (showPath) {
            print_hunt_path(path, map);
        }
        cout << "Treasure found at " << treasureRow << "," << treasureCol << " with path length " << pathSize << ".\n";
    }
    else {
        if (showStats) {
            print_stats(foundTreasure);
        }
        int totalLocations = discoveredWaterCount + discoveredLandCount;
        cout << "No treasure found after investigating " << totalLocations << " locations.\n";
    }
}

/**
 *  Main function.
 *
 */
int main(int argc, char** argv) {
    ios_base::sync_with_stdio(false);
    xcode_redirect(argc, argv);
    
    // get the options
    get_options(argc, argv);
    
    // read data from input file
    vector<vector<MapGrid> > map;
    read_data(map);
    
    // hunt for the treasure!
    bool foundTreasure = hunt_treasure(map);
    
    // print the hunt logs
    print_hunt_logs(foundTreasure, map);
    
    return 0;
}
