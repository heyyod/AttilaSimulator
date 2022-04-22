/***********************************************************							***************
 *
 * Copyright (c) 2002 - 2011 by Computer Architecture Department,
 * Universitat Politecnica de Catalunya.
 * All rights reserved.
 *
 * The contents of this file may not be disclosed to third parties,
 * copied or duplicated in any form, in whole or in part, without the
 * prior permission of the authors, Computer Architecture Department
 * and Universitat Politecnica de Catalunya.
 *
 */

#include "StatisticsManager.h"
#include "support.h"
#include <algorithm>

using namespace gpu3d;
using namespace gpu3d::GPUStatistics;
using namespace std;

// StatisticsManager* StatisticsManager::sm = 0;

StatisticsManager::StatisticsManager() :
    startCycle(0), nCycles(1000), nextDump(999), lastCycle(-1), autoReset(true),
    osCycle(NULL), osFrame(NULL), osBatch(NULL), cyclesFlagNamesDumped(false)
{
	InitializeOutputFiles();
}

StatisticsManager& StatisticsManager::instance()
{
    static StatisticsManager* sm = new StatisticsManager();
    return *sm;
}

Statistic* StatisticsManager::find(std::string name)
{
    map<string,Statistic*>::iterator it = stats.find(name);
    if ( it == stats.end() )
        return 0;
    return it->second;
}

void StatisticsManager::clock(u64bit cycle)
{
    // static bool flag = false;
    lastCycle = cycle;

    if ( cycle >= startCycle )
        Statistic::enable();
    else
        Statistic::disable();

    if ( cycle >= nextDump )
    {
        if ( !cyclesFlagNamesDumped )
        {
            cyclesFlagNamesDumped = true;
            
            if (osCycle != NULL)
                dumpNames(*osCycle);
        }

        if (osCycle != NULL)
            dumpValues(*osCycle);

        if ( autoReset )
        {
            startCycle = cycle+1;
            reset(FREQ_CYCLES);
        }
        nextDump = cycle + nCycles;
    }
}

void StatisticsManager::frame(u32bit frame)
{
    static bool namesOut = false;

    //  Check if output stream for per frame statistics is defined.
    if (osFrame != NULL)
    {
        if (namesOut == false)
        {
            namesOut = true;
            dumpNames("Frame", *osFrame);
        }

        dumpValues(frame, FREQ_FRAME, *osFrame);

        reset(FREQ_FRAME);
    }
}

void StatisticsManager::batch()
{
    static bool namesOut = false;
    static u32bit batch = 0;

    //  Check if the output stream for per batch statistics is defined
    if (osBatch != NULL)
    {
        if (namesOut == false)
        {
            namesOut = true;
            dumpNames("Batch", *osBatch);
        }

        dumpValues(batch, FREQ_BATCH, *osBatch);

        batch++;

        reset(FREQ_BATCH);
    }
}

Statistic* StatisticsManager::operator[](std::string statName)
{
    return find(statName);
}


void StatisticsManager::setDumpScheduling(u64bit startCycle, u64bit nCycles, bool autoReset)
{
    this->startCycle = startCycle;
    this->nextDump= startCycle+nCycles-1;
    this->nCycles = nCycles; /* dump every 'nCycles' cycles */
    this->autoReset = autoReset;
}

void StatisticsManager::setOutputStream(ostream& os)
{
    osCycle = &os;
}

void StatisticsManager::setPerFrameStream(ostream& os)
{
    osFrame = &os;
}

void StatisticsManager::setPerBatchStream(ostream& os)
{
    osBatch = &os;
}

void StatisticsManager::reset(u32bit freq)
{
    map<string,Statistic*>::iterator it = stats.begin();
    for ( ; it != stats.end(); it++ )
        (it->second)->clear(freq);
}

void StatisticsManager::dumpValues(ostream& os)
{
    map<string,Statistic*>::iterator it = stats.begin();

    os << startCycle << ".." << lastCycle;
    for ( ; it != stats.end(); it++ )
    {
        (*(it->second)).setCurrentFreq(FREQ_CYCLES);
        os << ";" << *(it->second);
    }
    os << endl;
}

void StatisticsManager::dumpValues(u32bit n, u32bit freq, ostream& os)
{
    map<string,Statistic*>::iterator it = stats.begin();

    os << n;
    for ( ; it != stats.end(); it++ )
    {
        (*(it->second)).setCurrentFreq(freq);
        os << ";" << *(it->second);
    }

    os << endl;
}


void StatisticsManager::dumpNames(ostream& os)
{
    map<string,Statistic*>::iterator it = stats.begin();

    os << "Cycles";
    for ( ; it != stats.end(); it++ )
        os << ";" << it->first;
    os << endl;
}

void StatisticsManager::dumpNames(char *str, ostream& os)
{
    map<string,Statistic*>::iterator it = stats.begin();

    os << str;
    for ( ; it != stats.end(); it++ )
        os << ";" << it->first;
    os << endl;
}

void StatisticsManager::dump(ostream& os)
{
    map<string,Statistic*>::iterator it = stats.begin();
    for ( ; it != stats.end(); it++ )
    {
        os << it->second->getName() << " = " << *(it->second) << endl;
    }
}


void StatisticsManager::dump(string boxName, ostream& os)
{
    map<string,Statistic*>::iterator it = stats.begin();
    for ( ; it != stats.end(); it++ )
    {
        if ( it->second->getOwner() == boxName )
            os << it->second->getName() << " = " << *(it->second) << endl;
    }

}

void StatisticsManager::finish()
{
    u64bit prevDump = nextDump + 1 - nCycles;
    if ( lastCycle > prevDump ) {
        if ( osCycle ) {
            if ( !cyclesFlagNamesDumped )
                dumpNames(*osCycle);
            dumpValues(*osCycle);
        }    
    }
}

#if KONDAMASK
void StatisticsManager::LogCacheAccess(char* name, u64bit address, CACHE_LOG_INFO logInfo, u32bit set, u32bit way,
	u64bit thisCycle, u64bit insertCycle, u64bit lastHitCycle, u64bit lastOnCycle, bool isReplace)
{
	if (!KONDAMASK_CACHE_LOG_CSV)
		return;

	address &= 0xffffffff;
	/*
		Columns :
		- Cycle
		- Cache Unit
		- Address
		- Hit/Miss
		- Set
		- Way
		- InsertToHitCycles
		- HitToHitCycles
		- HitToReplaceCycles / Dead Time
		- CacheOffCycles
		*/

#define Column(name)    name << ';'
#define ColumnEmpty()   ';'

	osCacheAccesses <<
		Column(thisCycle) <<
		Column(name) <<
		Column(address);

	switch (logInfo)
	{
		case CACHE_FETCH_HIT:
		{
			osCacheAccesses << Column("HIT");
		} break;
		case CACHE_FETCH_MISS:
		{
			osCacheAccesses << Column("MISS");
		} break;
		case CACHE_FETCH_FAIL:
		{
			osCacheAccesses << Column("FAIL");
		} break;
		case CACHE_DECAY:
		{
			osCacheAccesses << Column("DECAY");
		} break;
		case CACHE_DECAY_RESERVED:
		{
			osCacheAccesses << Column("DECAY-FAIL-RESERVED");
		} break;
		case CACHE_DECAY_REPLACING:
		{
			osCacheAccesses << Column("DECAY-FAIL-REPLACING");
		} break;
		case CACHE_WRITE_REQUEST:
		{
			osCacheAccesses << Column("WRITE-REQUEST");
		} break;
		case CACHE_READ_REQUEST:
		{
			osCacheAccesses << Column("READ-REQUEST");
		} break;
		case CACHE_WRITE_TRANS:
		{
			osCacheAccesses << Column("WRITE-TRANS");
		} break;
		case CACHE_READ_TRANS:
		{
			osCacheAccesses << Column("READ-TRANS");
		} break;
		case CACHE_DECAY_FLUSH_FAILED:
		{
			osCacheAccesses << Column("FLUSH-FAILED");
		} break;


	}
	osCacheAccesses <<
		Column(set) <<
		Column(way);

	if (logInfo == CACHE_FETCH_HIT)
	{
		osCacheAccesses <<
			Column(thisCycle - insertCycle) <<
			Column(thisCycle - lastHitCycle) <<
			ColumnEmpty() <<
			ColumnEmpty();
	}
	else if (logInfo == CACHE_DECAY || (logInfo == CACHE_FETCH_MISS && isReplace))
	{
		osCacheAccesses <<
			ColumnEmpty() <<
			ColumnEmpty() <<
			Column(thisCycle - lastHitCycle) <<
			ColumnEmpty();
	}
	else if (
		logInfo == CACHE_DECAY_RESERVED ||
		logInfo == CACHE_DECAY_REPLACING ||
		logInfo == CACHE_FETCH_MISS)
	{
		osCacheAccesses <<
			ColumnEmpty() <<
			ColumnEmpty() <<
			ColumnEmpty() <<
			Column(thisCycle - lastOnCycle);
	}
	osCacheAccesses << std::endl;
}

void StatisticsManager::LogFrameDecayStats(char* name, u32bit frame, u32bit frameCycles, cache_decay_stats *stats, u32bit count)
{
	// In stats:
	// 1 In
	// 4 Tex L0
	// 4 Tex L1
	// 4 Z
	// 4 Color
	// 17 Total
	// +
	// texL0Avg, texL1Avg, zAvg, Color Avg
	// 21 Total
	
	/*
	* osDecayStats << "
	* Trace;Frame;Cycles;
	* InDecay;InOff;InDecayedRefetches;
	* TexL0Decay;TexL1Decay;
	* TexL0Off0;TexL0DecayedRefetches;TexL1Off0;TexL0DecayedRefetches;TexL0Off1;TexL0DecayedRefetches;TexL1Off1;TexL0DecayedRefetches;TexL0Off2;TexL0DecayedRefetches;TexL1Off2;TexL0DecayedRefetches;TexL0Off3;TexL0DecayedRefetches;TexL1Off3;TexL0DecayedRefetches;TexL0OffAvg;TexL0DecayedRefetchesAvg;TexL1OffAvg;TexL1DecayedRefetches;ZDecay;ZOff0;ZDecayedRefetches0;ZOff1;ZDecayedRefetches1;ZOff2;ZDecayedRefetches2;ZOff3;ZDecayedRefetches3;ZOffAvg;ZDecayedRefetchesAvg;ColorDecay;ColorOff0;ColorDecayedRefetches0;ColorOff1;ColorDecayedRefetches1;ColorOff2;ColorDecayedRefetches2;ColorOff3;ColorDecayedRefetches3;ColorOffAvg;ColorDecayedRefetchesAvg;" << std::endl;
	*/
	//              Trace           Frame            Cycles;
	osDecayStats << Column(name) << Column(frame) << Column(frameCycles);
	
	//              InDecay                         InOff                             InDecayedRefetches;
	osDecayStats << Column(stats[0].decayCycles) << Column(stats[0].offPercentage) << Column(stats[0].decayedRefetchesPercentage);
	
	//              TexL0Decay                      TexL1Decay
	osDecayStats << Column(stats[1].decayCycles) << Column(stats[2].decayCycles);
	
	u32bit start = 1;
	for (u32bit i = start; i < start + 10; i += 2)
	{
		//              TexL0Offi                         TexL0DecayedRefetchesi;
		osDecayStats << Column(stats[i].offPercentage) << Column(stats[i].decayedRefetchesPercentage);
		
		//              TexL1Offi                           TexL1DecayedRefetchesi;
		osDecayStats << Column(stats[i+1].offPercentage) << Column(stats[i+1].decayedRefetchesPercentage);
	}
	
	start = 11;  // Z Decay cycles
	osDecayStats << Column(stats[start].decayCycles);
	for (u32bit i = start; i < start + 5; i += 1)
	{
		//              ZOffi                         ZDecayedRefetchesi;
		osDecayStats << Column(stats[i].offPercentage) << Column(stats[i].decayedRefetchesPercentage);
	}
	
	start = 16;		// Color decay Cycles
	osDecayStats << Column(stats[start].decayCycles);
	for (u32bit i = start; i < start + 5; i += 1)
	{
		//              ColorOffi                         ColorDecayedRefetchesi;
		osDecayStats << Column(stats[i].offPercentage) << Column(stats[i].decayedRefetchesPercentage);
	}
	
	osDecayStats << std::endl;
}

void StatisticsManager::InitializeOutputFiles()
{
	if (KONDAMASK_CACHE_LOG_CSV)
	{
		osCacheAccesses.open("out/CacheAccesses.csv", std::ofstream::out | std::ofstream::trunc);
		if (!osCacheAccesses.is_open())
		{
			printf("Couldn't open out/CacheAccesses.csv.\n");
			return;
		}
		osCacheAccesses << "Cycle;Cache Unit;Address;Hit/Miss;Set;Way;InsertToHitCycles;HitToHitCycles;CacheIdleCycles;CacheOffCycles" << std::endl;
	}

	osDecayStats.open("out/DecayStats.csv", std::ofstream::out | std::ofstream::trunc);
	if (!osDecayStats.is_open())
	{
		printf("Couldn't open out/DecayStats.csv.\n");
		return;
	}
	osDecayStats << "Trace;Frame;Cycles;InDecay;InOff;InDecayedRefetches;TexL0Decay;TexL1Decay;TexL0Off0;TexL0DecayedRefetches0;TexL1Off0;TexL0DecayedRefetches0;TexL0Off1;TexL0DecayedRefetches1;TexL1Off1;TexL0DecayedRefetches1;TexL0Off2;TexL0DecayedRefetches2;TexL1Off2;TexL0DecayedRefetches2;TexL0Off3;TexL0DecayedRefetches3;TexL1Off3;TexL0DecayedRefetches3;TexL0OffAvg;TexL0DecayedRefetchesAvg;TexL1OffAvg;TexL1DecayedRefetchesAvg;ZDecay;ZOff0;ZDecayedRefetches0;ZOff1;ZDecayedRefetches1;ZOff2;ZDecayedRefetches2;ZOff3;ZDecayedRefetches3;ZOffAvg;ZDecayedRefetchesAvg;ColorDecay;ColorOff0;ColorDecayedRefetches0;ColorOff1;ColorDecayedRefetches1;ColorOff2;ColorDecayedRefetches2;ColorOff3;ColorDecayedRefetches3;ColorOffAvg;ColorDecayedRefetchesAvg;" << std::endl;
}

void StatisticsManager::SaveOutputFiles()
{
	if (osCacheAccesses.is_open())
	{
		osCacheAccesses.close();
	}

	if (osDecayStats.is_open())
	{
		osDecayStats.close();
	}
}
#endif