#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
using namespace std;

struct Cache
{
    char state; // M, O, E, S, I, F
    int lru;    // LRU state
    bool dirty; // Dirty bit
    int tag;    // Tag
};

class Core
{
public:
    Cache caches[4]; // 4 caches per core

    Core()
    {
        // Start in I state, with lru=index, dirty=0, tag=0
        for (int i = 0; i < 4; ++i)
        {
            caches[i].state = 'I';
            caches[i].lru = i;
            caches[i].dirty = false;
            caches[i].tag = 0;
        }
    }

    int findReplacementLine()
    {
        for (int i = 0; i < 4; ++i)
        {
            if (caches[i].state == 'I')
            {
                return i; // Return first invalid line
            }
        }
        // If no invalid line, find the line with LRU state 0
        for (int i = 0; i < 4; ++i)
        {
            if (caches[i].lru == 0)
            {
                return i; // Return least recently used line
            }
        }
        return -1;
    }

    void updateLRU(int accessedIndex)
    {
        int currentLRU = caches[accessedIndex].lru;
        for (int i = 0; i < 4; ++i)
        {
            if (caches[i].lru > currentLRU)
            {
                caches[i].lru--;
            }
        }
        caches[accessedIndex].lru = 3; // Most recently used
    }

    void makeLRU(int accessedIndex)
    {
        int currentLRU = caches[accessedIndex].lru;
        for (int i = 0; i < 4; ++i)
        {
            if (caches[i].lru < currentLRU)
            {
                // Increment LRU for lines with a lower LRU value
                caches[i].lru++;
            }
        }
        // Set the accessed line to be the least recently used
        caches[accessedIndex].lru = 0;
    }

    void print()
    {
        for (int i = 0; i < 4; ++i)
        {
            cout << "Cache Line " << i << ": "
                 << "State=" << caches[i].state << ", "
                 << "LRU=" << caches[i].lru << ", "
                 << "Dirty=" << (caches[i].dirty ? "true" : "false") << ", "
                 << "Tag=" << caches[i].tag << endl;
        }
        cout << endl;
    }
};

class MOESIFSimulator
{
private:
    Core cores[4];
    int cacheHits, cacheMisses, writebacks, broadcasts, cacheToCacheTransfers;

public:
    MOESIFSimulator() : cacheHits(0), cacheMisses(0), writebacks(0), broadcasts(0), cacheToCacheTransfers(0) {}

    bool tagInOtherCores(int coreID, int tag)
    {
        for (int i = 0; i < 4; ++i)
        {
            if (i == coreID)
                continue;

            for (int j = 0; j < 4; ++j)
            {
                if (cores[i].caches[j].tag == tag)
                    return true;
            }
        }

        return false;
    }

    bool tagInOtherValidCores(int coreID, int tag)
    {
        for (int i = 0; i < 4; ++i)
        {
            if (i == coreID)
                continue;

            for (int j = 0; j < 4; ++j)
            {
                if (cores[i].caches[j].tag == tag && cores[i].caches[j].state != 'I' && cores[i].caches[j].state != 'S')
                    return true;
            }
        }

        return false;
    }

    void processCommand(string command, int coreID, int tag)
    {
        // cout << "P" << coreID + 1 << ": " << command << " <" << tag << ">" << endl;
        // Access the requesting core
        Core &reqCore = cores[coreID];
        int lineIndex = -1;

        // Look for the tag in the cache
        for (int i = 0; i < 4; ++i)
        {
            if (reqCore.caches[i].tag == tag)
            {
                lineIndex = i;
                break;
            }
        }

        bool tagFound = (lineIndex != -1);

        if (tagFound)
        {
            // Update cache hit
            if (reqCore.caches[lineIndex].state != 'I')
            {
                cacheHits++;
            }
            else
            {
                cacheMisses++;
                if (command == "read" && tagInOtherValidCores(coreID, tag))
                    cacheToCacheTransfers++;
            }

            // Update broadcasts
            if (reqCore.caches[lineIndex].state != 'E')
                broadcasts++;

            if (command == "read")
            {
                if (reqCore.caches[lineIndex].state == 'I' && !tagInOtherCores(coreID, tag))
                {
                    // If other cores have the tag but they're I, disregard them
                    reqCore.caches[lineIndex].state = 'E';
                }
                else if (reqCore.caches[lineIndex].state != 'M')
                {
                    reqCore.caches[lineIndex].state = 'S';
                }
            }
            else if (command == "write")
            {
                if (reqCore.caches[lineIndex].state == 'O')
                    writebacks++;

                reqCore.caches[lineIndex].state = 'M';
                reqCore.caches[lineIndex].dirty = true;
            }

            reqCore.updateLRU(lineIndex);
        }
        else
        {
            cacheMisses++;
            broadcasts++;

            // Install the new line
            int replacementIndex = reqCore.findReplacementLine();
            Cache &cache = reqCore.caches[replacementIndex];

            // Writeback if the line is dirty
            if (cache.dirty)
                writebacks++;

            // Instantiate correct values for the line
            cache.tag = tag;
            cache.dirty = (command == "write");

            if (command == "read")
            {
                if (tagInOtherValidCores(coreID, tag))
                    cacheToCacheTransfers++;

                if (tagInOtherCores(coreID, tag))
                {
                    cache.state = 'S';
                }
                else
                {
                    cache.state = 'E';
                }
            }
            else
            {
                cache.state = 'M';
            }

            reqCore.updateLRU(replacementIndex);
        }

        // Update other caches
        for (int id = 0; id < 4; ++id)
        {
            if (id == coreID)
                continue;

            Core &otherCore = cores[id];
            for (int i = 0; i < 4; ++i)
            {
                Cache &otherCache = otherCore.caches[i];

                if (otherCache.tag == tag)
                {
                    if (command == "read")
                    {
                        // E goes to F
                        if (otherCache.state == 'E')
                        {
                            otherCache.state = 'F';
                        }
                        // M goes to O
                        else if (otherCache.state == 'M')
                        {
                            otherCache.state = 'O';
                        }
                    }
                    else if (command == "write")
                    {
                        // If a line needs to be invalidated, check the dirty bit and issue a writeback
                        if (otherCache.dirty)
                        {
                            writebacks++;
                            otherCache.dirty = false;
                        }
                        otherCache.state = 'I';
                        otherCore.makeLRU(i);
                    }
                }
            }
        }
    }

    void simulate(string inputFile)
    {
        ifstream file(inputFile);
        string line;

        // Parse input file
        while (getline(file, line))
        {
            stringstream ss(line);
            string coreStr, op, tagStr;
            ss >> coreStr >> op >> tagStr;

            int coreID = coreStr[1] - '0';
            int tag = stoi(tagStr.substr(1, tagStr.size() - 2));

            // Zero index the coreID
            processCommand(op, coreID - 1, tag);
        }

        // Print results
        cout << cacheHits << endl;
        cout << cacheMisses << endl;
        cout << writebacks << endl;
        cout << broadcasts << endl;
        cout << cacheToCacheTransfers;
    }
};

int main(int argc, char *argv[])
{
    // g++ *.cpp -o coherentsim
    // ./coherentsim <inputfile.txt>
    if (argc != 2)
    {
        cerr << "Usage: " << argv[0] << " <inputfile.txt>" << endl;
        return 1;
    }

    MOESIFSimulator simulator;
    simulator.simulate(argv[1]);

    return 0;
}
