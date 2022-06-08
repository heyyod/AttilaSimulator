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
	map<string, Statistic*>::iterator it = stats.find(name);
	if (it == stats.end())
		return 0;
	return it->second;
}

void StatisticsManager::clock(u64bit cycle)
{
	// static bool flag = false;
	lastCycle = cycle;

	if (cycle >= startCycle)
		Statistic::enable();
	else
		Statistic::disable();

	if (cycle >= nextDump)
	{
		if (!cyclesFlagNamesDumped)
		{
			cyclesFlagNamesDumped = true;

			if (osCycle != NULL)
				dumpNames(*osCycle);
		}

		if (osCycle != NULL)
			dumpValues(*osCycle);

		if (autoReset)
		{
			startCycle = cycle + 1;
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
	this->nextDump = startCycle + nCycles-1;
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
	map<string, Statistic*>::iterator it = stats.begin();
	for (; it != stats.end(); it++)
		(it->second)->clear(freq);
}

void StatisticsManager::dumpValues(ostream& os)
{
	map<string, Statistic*>::iterator it = stats.begin();

	os << startCycle << ".." << lastCycle;
	for (; it != stats.end(); it++)
	{
		(*(it->second)).setCurrentFreq(FREQ_CYCLES);
		os << ";" << *(it->second);
	}
	os << endl;
}

void StatisticsManager::dumpValues(u32bit n, u32bit freq, ostream& os)
{
	map<string, Statistic*>::iterator it = stats.begin();

	os << n;
	for (; it != stats.end(); it++)
	{
		(*(it->second)).setCurrentFreq(freq);
		os << ";" << *(it->second);
	}

	os << endl;
}


void StatisticsManager::dumpNames(ostream& os)
{
	map<string, Statistic*>::iterator it = stats.begin();

	os << "Cycles";
	for (; it != stats.end(); it++)
		os << ";" << it->first;
	os << endl;
}

void StatisticsManager::dumpNames(char *str, ostream& os)
{
	map<string, Statistic*>::iterator it = stats.begin();

	os << str;
	for (; it != stats.end(); it++)
		os << ";" << it->first;
	os << endl;
}

void StatisticsManager::dump(ostream& os)
{
	map<string, Statistic*>::iterator it = stats.begin();
	for (; it != stats.end(); it++)
	{
		os << it->second->getName() << " = " << *(it->second) << endl;
	}
}


void StatisticsManager::dump(string boxName, ostream& os)
{
	map<string, Statistic*>::iterator it = stats.begin();
	for (; it != stats.end(); it++)
	{
		if (it->second->getOwner() == boxName)
			os << it->second->getName() << " = " << *(it->second) << endl;
	}

}

void StatisticsManager::finish()
{
	u64bit prevDump = nextDump + 1 - nCycles;
	if (lastCycle > prevDump) {
		if (osCycle) {
			if (!cyclesFlagNamesDumped)
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
	GPUStatistics::StatisticsManager::cache_decay_stats &in = stats[0];
	GPUStatistics::StatisticsManager::cache_decay_stats &tex0 = stats[1];
	GPUStatistics::StatisticsManager::cache_decay_stats &tex1 = stats[3];
	GPUStatistics::StatisticsManager::cache_decay_stats &tex2 = stats[5];
	GPUStatistics::StatisticsManager::cache_decay_stats &tex3 = stats[7];
	GPUStatistics::StatisticsManager::cache_decay_stats &texL0Avg = stats[9];
	GPUStatistics::StatisticsManager::cache_decay_stats &texL1Avg = stats[10];
	GPUStatistics::StatisticsManager::cache_decay_stats &z0 = stats[11];
	GPUStatistics::StatisticsManager::cache_decay_stats &z1 = stats[12];
	GPUStatistics::StatisticsManager::cache_decay_stats &z2 = stats[13];
	GPUStatistics::StatisticsManager::cache_decay_stats &z3 = stats[14];
	GPUStatistics::StatisticsManager::cache_decay_stats &zAvg = stats[15];
	GPUStatistics::StatisticsManager::cache_decay_stats &color0 = stats[16];
	GPUStatistics::StatisticsManager::cache_decay_stats &color1 = stats[17];
	GPUStatistics::StatisticsManager::cache_decay_stats &color2 = stats[18];
	GPUStatistics::StatisticsManager::cache_decay_stats &color3 = stats[19];
	GPUStatistics::StatisticsManager::cache_decay_stats &colorAvg = stats[20];

	//------------------------------------------------------------------------
	texL0Avg.decayInterval = (&tex0)[0].decayInterval;
	texL0Avg.offTime = ((&tex0)[0].offTime + (&tex1)[0].offTime + (&tex2)[0].offTime + (&tex3)[0].offTime) / 4.0;
	texL0Avg.idleTime = ((&tex0)[0].idleTime + (&tex1)[0].idleTime + (&tex2)[0].idleTime + (&tex3)[0].idleTime) / 4.0;
	texL0Avg.activeTime = ((&tex0)[0].activeTime + (&tex1)[0].activeTime + (&tex2)[0].activeTime + (&tex3)[0].activeTime) / 4.0;
	texL0Avg.hits = ((&tex0)[0].hits + (&tex1)[0].hits + (&tex2)[0].hits + (&tex3)[0].hits) / 4.0;
	texL0Avg.misses = ((&tex0)[0].misses + (&tex1)[0].misses + (&tex2)[0].misses + (&tex3)[0].misses) / 4.0;
	
	//------------------------------------------------------------------------
	texL1Avg.decayInterval = (&tex0)[1].decayInterval;
	texL1Avg.offTime = ((&tex0)[1].offTime + (&tex1)[1].offTime + (&tex2)[1].offTime + (&tex3)[1].offTime) / 4.0;
	texL1Avg.idleTime = ((&tex0)[1].idleTime + (&tex1)[1].idleTime + (&tex2)[1].idleTime + (&tex3)[1].idleTime) / 4.0;
	texL1Avg.activeTime = ((&tex0)[1].activeTime + (&tex1)[1].activeTime + (&tex2)[1].activeTime + (&tex3)[1].activeTime) / 4.0; texL1Avg.hits = ((&tex0)[1].hits + (&tex1)[1].hits + (&tex2)[1].hits + (&tex3)[1].hits) / 4.0;
	texL1Avg.misses = ((&tex0)[1].misses + (&tex1)[1].misses + (&tex2)[1].misses + (&tex3)[1].misses) / 4.0;

	//------------------------------------------------------------------------
	zAvg.decayInterval = z0.decayInterval;
	zAvg.offTime = (z0.offTime + z1.offTime + z2.offTime + z3.offTime) / 4.0;
	zAvg.idleTime = (z0.idleTime + z1.idleTime + z2.idleTime + z3.idleTime) / 4.0;
	zAvg.activeTime = (z0.activeTime + z1.activeTime + z2.activeTime + z3.activeTime) / 4.0;
	zAvg.hits = (z0.hits + z1.hits + z2.hits + z3.hits) / 4.0;
	zAvg.misses = (z0.misses + z1.misses + z2.misses + z3.misses) / 4.0;

	//------------------------------------------------------------------------
	colorAvg.decayInterval = color0.decayInterval;
	colorAvg.offTime = (color0.offTime + color1.offTime + color2.offTime + color3.offTime) / 4.0;
	colorAvg.idleTime = (color0.idleTime + color1.idleTime + color2.idleTime + color3.idleTime) / 4.0;
	colorAvg.activeTime = (color0.activeTime + color1.activeTime + color2.activeTime + color3.activeTime) / 4.0;
	colorAvg.hits = (color0.hits + color1.hits + color2.hits + color3.hits) / 4.0;
	colorAvg.misses = (color0.misses + color1.misses + color2.misses + color3.misses) / 4.0;

	//------------------------------------------------------------------------
	
	osDecayStats << 
		Column(name) << 
		Column(frame) << 
		Column(frameCycles) <<
		Column(in.decayInterval) <<
		Column(texL0Avg.decayInterval) <<
		Column(texL1Avg.decayInterval) <<
		Column(zAvg.decayInterval) <<
		Column(colorAvg.decayInterval);
                  
	osDecayStats << 
		Column(in.hits) <<
		Column(in.misses) <<
		Column(in.offTime) <<
		Column(in.idleTime) <<
		Column(in.activeTime);
	
	osDecayStats << 
		Column(texL0Avg.hits) <<
		Column(texL0Avg.misses) <<
		Column(texL0Avg.offTime) <<
		Column(texL0Avg.idleTime) <<
		Column(texL0Avg.activeTime);
	
	osDecayStats << 
		Column(texL1Avg.hits) <<
		Column(texL1Avg.misses) <<
		Column(texL1Avg.offTime) <<
		Column(texL1Avg.idleTime) <<
		Column(texL1Avg.activeTime);
	
	osDecayStats << 
		Column(zAvg.hits) <<
		Column(zAvg.misses) <<
		Column(zAvg.offTime) <<
		Column(zAvg.idleTime) <<
		Column(zAvg.activeTime);
	
	osDecayStats << 
		Column(colorAvg.hits) <<
		Column(colorAvg.misses) <<
		Column(colorAvg.offTime) <<
		Column(colorAvg.idleTime) <<
		Column(colorAvg.activeTime);
	
	//------------------------------------------------------------------------
	
	osDecayStats << 
		Column((&tex0)[0].hits) <<
		Column((&tex0)[0].misses) <<
		Column((&tex0)[0].offTime) <<
		Column((&tex0)[0].idleTime) <<
		Column((&tex0)[0].activeTime);
	
	osDecayStats << 
		Column((&tex1)[0].hits) <<
		Column((&tex1)[0].misses) <<
		Column((&tex1)[0].offTime) <<
		Column((&tex1)[0].idleTime) <<
		Column((&tex1)[0].activeTime);
	
	osDecayStats << 
		Column((&tex2)[0].hits) <<
		Column((&tex2)[0].misses) <<
		Column((&tex2)[0].offTime) <<
		Column((&tex2)[0].idleTime) <<
		Column((&tex2)[0].activeTime);
	
	osDecayStats << 
		Column((&tex3)[0].hits) <<
		Column((&tex3)[0].misses) <<
		Column((&tex3)[0].offTime) <<
		Column((&tex3)[0].idleTime) <<
		Column((&tex3)[0].activeTime);
	
	//------------------------------------------------------------------------
	
	osDecayStats << 
		Column((&tex0)[1].hits) <<
		Column((&tex0)[1].misses) <<
		Column((&tex0)[1].offTime) <<
		Column((&tex0)[1].idleTime) <<
		Column((&tex0)[1].activeTime);
	
	osDecayStats << 
		Column((&tex1)[1].hits) <<
		Column((&tex1)[1].misses) <<
		Column((&tex1)[1].offTime) <<
		Column((&tex1)[1].idleTime) <<
		Column((&tex1)[1].activeTime);
	
	osDecayStats << 
		Column((&tex2)[1].hits) <<
		Column((&tex2)[1].misses) <<
		Column((&tex2)[1].offTime) <<
		Column((&tex2)[1].idleTime) <<
		Column((&tex2)[1].activeTime);
	
	osDecayStats << 
		Column((&tex3)[1].hits) <<
		Column((&tex3)[1].misses) <<
		Column((&tex3)[1].offTime) <<
		Column((&tex3)[1].idleTime) <<
		Column((&tex3)[1].activeTime);
	
	//------------------------------------------------------------------------
	
	osDecayStats << 
		Column(z0.hits) <<
		Column(z0.misses) <<
		Column(z0.offTime) <<
		Column(z0.idleTime) <<
		Column(z0.activeTime);
	
	osDecayStats << 
		Column(z1.hits) <<
		Column(z1.misses) <<
		Column(z1.offTime) <<
		Column(z1.idleTime) <<
		Column(z1.activeTime);
	
	osDecayStats << 
		Column(z2.hits) <<
		Column(z2.misses) <<
		Column(z2.offTime) <<
		Column(z2.idleTime) <<
		Column(z2.activeTime);
	
	osDecayStats << 
		Column(z3.hits) <<
		Column(z3.misses) <<
		Column(z3.offTime) <<
		Column(z3.idleTime) <<
		Column(z3.activeTime);
	
	//------------------------------------------------------------------------
	
	osDecayStats << 
		Column(color0.hits) <<
		Column(color0.misses) <<
		Column(color0.offTime) <<
		Column(color0.idleTime) <<
		Column(color0.activeTime);
	
	osDecayStats << 
		Column(color1.hits) <<
		Column(color1.misses) <<
		Column(color1.offTime) <<
		Column(color1.idleTime) <<
		Column(color1.activeTime);
	
	osDecayStats << 
		Column(color2.hits) <<
		Column(color2.misses) <<
		Column(color2.offTime) <<
		Column(color2.idleTime) <<
		Column(color2.activeTime);
	
	osDecayStats << 
		Column(color3.hits) <<
		Column(color3.misses) <<
		Column(color3.offTime) <<
		Column(color3.idleTime) <<
		Column(color3.activeTime);
	
	//------------------------------------------------------------------------

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
	osDecayStats <<
		Column("trace") <<
		Column("frame") <<
		Column("cycles") <<
		Column("in decay") <<
		Column("texL0 decay") <<
		Column("texL1 decay") <<
		Column("z decay") <<
		Column("color decay") <<
		Column("in hits") <<
		Column("in misses") <<
		Column("in off time") <<
		Column("in idle time") <<
		Column("in active time") <<
		Column("texL0 hits") <<
		Column("texL0 misses") <<
		Column("texL0 off time") <<
		Column("texL0 idle time") <<
		Column("texL0 active time") <<
		Column("texL1 hits") <<
		Column("texL1 misses") <<
		Column("texL1 off time") <<
		Column("texL1 idle time") <<
		Column("texL1 active time") <<
		Column("Z hits") <<
		Column("Z misses") <<
		Column("Z off time") <<
		Column("Z idle time") <<
		Column("Z active time") <<
		Column("Color hits") <<
		Column("Color misses") <<
		Column("Color off time") <<
		Column("Color idle time") <<
		Column("Color active time") <<
		Column("texL0_0 hits") <<
		Column("texL0_0 misses") <<
		Column("texL0_0 off time") <<
		Column("texL0_0 idle time") <<
		Column("texL0_0 active time") <<
		Column("texL0_1 hits") <<
		Column("texL0_1 misses") <<
		Column("texL0_1 off time") <<
		Column("texL0_1 idle time") <<
		Column("texL0_1 active time") <<
		Column("texL0_2 hits") <<
		Column("texL0_2 misses") <<
		Column("texL0_2 off time") <<
		Column("texL0_2 idle time") <<
		Column("texL0_2 active time") <<
		Column("texL0_3 hits") <<
		Column("texL0_3 misses") <<
		Column("texL0_3 off time") <<
		Column("texL0_3 idle time") <<
		Column("texL0_3 active time") <<
		Column("texL1_0 hits") <<
		Column("texL1_0 misses") <<
		Column("texL1_0 off time") <<
		Column("texL1_0 idle time") <<
		Column("texL1_0 active time") <<
		Column("texL1_1 hits") <<
		Column("texL1_1 misses") <<
		Column("texL1_1 off time") <<
		Column("texL1_1 idle time") <<
		Column("texL1_1 active time") <<
		Column("texL1_2 hits") <<
		Column("texL1_2 misses") <<
		Column("texL1_2 off time") <<
		Column("texL1_2 idle time") <<
		Column("texL1_2 active time") <<
		Column("texL1_3 hits") <<
		Column("texL1_3 misses") <<
		Column("texL1_3 off time") <<
		Column("texL1_3 idle time") <<
		Column("texL1_3 active time") <<
		Column("z_0 hits") <<
		Column("z_0 misses") <<
		Column("z_0 off time") <<
		Column("z_0 idle time") <<
		Column("z_0 active time") <<
		Column("z_1 hits") <<
		Column("z_1 misses") <<
		Column("z_1 off time") <<
		Column("z_1 idle time") <<
		Column("z_1 active time") <<
		Column("z_2 hits") <<
		Column("z_2 misses") <<
		Column("z_2 off time") <<
		Column("z_2 idle time") <<
		Column("z_2 active time") <<
		Column("z_3 hits") <<
		Column("z_3 misses") <<
		Column("z_3 off time") <<
		Column("z_3 idle time") <<
		Column("z_3 active time") <<
		Column("color_0 hits") <<
		Column("color_0 misses") <<
		Column("color_0 off time") <<
		Column("color_0 idle time") <<
		Column("color_0 active time") <<
		Column("color_1 hits") <<
		Column("color_1 misses") <<
		Column("color_1 off time") <<
		Column("color_1 idle time") <<
		Column("color_1 active time") <<
		Column("color_2 hits") <<
		Column("color_2 misses") <<
		Column("color_2 off time") <<
		Column("color_2 idle time") <<
		Column("color_2 active time") <<
		Column("color_3 hits") <<
		Column("color_3 misses") <<
		Column("color_3 off time") <<
		Column("color_3 idle time") <<
		Column("color_3 active time") <<
		std::endl;
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