/**************************************************************************
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

#ifndef STATISTICSMANAGER_H
#define STATISTICSMANAGER_H

#include "Statistic.h"
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <stdio.h>
#include "support.h"
#include "GPUTypes.h"

#if KONDAMASK
#include <fstream>
#include <unordered_map>
#endif

namespace gpu3d
{

	namespace GPUStatistics
	{

		static const u32bit FREQ_CYCLES = 0;
		static const u32bit FREQ_FRAME = 1;
		static const u32bit FREQ_BATCH = 2;

		class StatisticsManager
		{
		private:

			bool cyclesFlagNamesDumped;

			/* list of current stats */
			std::map<std::string, GPUStatistics::Statistic*> stats;

			/* Helper method to find a Statistic with name 'name' */
			Statistic* find(std::string name);

			/* dump scheduling */
			u64bit startCycle;
			u64bit nCycles;
			u64bit nextDump;
			u64bit lastCycle;
			bool autoReset;

			/* current output per cycle stream */
			std::ostream* osCycle;

			/*  output streams for per batch and per frame statistics.  */
			std::ostream* osFrame;
			std::ostream* osBatch;

#if KONDAMASK_CACHE_ACCESSES_CSV
			std::ofstream osCacheAccesses;
#endif

			/* singleton instance */
			// static StatisticsManager* sm;

			/* avoid copying */
			StatisticsManager();
			StatisticsManager(const StatisticsManager&);
			StatisticsManager& operator=(const StatisticsManager&);

		public:

			static StatisticsManager& instance();


			template<typename T>
			GPUStatistics::NumericStatistic<T>& getNumericStatistic(const char* name, T initialValue, const char* owner = 0, const char* postfix = 0)
			{
				char temp[256];
				if (postfix != 0)
				{
					sprintf(temp, "%s_%s", name, postfix);
					name = temp;
				}
				Statistic* st = find(name);
				NumericStatistic<T>* nst;
				if (st)
				{
					/*
					 * Warning (in windows):
					 * Use this flag (-GR) in VS6 to achive RTTI in dynamic_cast
					 *
					 *  -GR    enable standard C++ RTTI (Run Time Type
					 *         Identification)
					 */
					nst = dynamic_cast<NumericStatistic<T>*>(st);

					if (nst == 0)
					{
						char temp[128];
						sprintf(temp, "Another Statistic exists with name '%s' but has different type", name);
						panic("StatisticsManager", "getNumericStatistic()", temp);
					}
					return *nst;
				}

				nst = new NumericStatistic<T>(name, initialValue);
				stats.insert(std::make_pair(name, nst));

				if (owner != 0)
					nst->setOwner(owner);

				return *nst;
			}

			Statistic* operator[](std::string statName);

			virtual void clock(u64bit cycle);

			virtual void frame(u32bit frame);

			virtual void batch();

			void setDumpScheduling(u64bit startCycle, u64bit nCycles, bool autoReset = true);

			void setOutputStream(std::ostream& os);

			void setPerFrameStream(std::ostream& os);

			void setPerBatchStream(std::ostream& os);

			void reset(u32bit freq);

			void dumpNames(std::ostream& os = std::cout);

			void dumpValues(std::ostream& os = std::cout);

			void dumpNames(char* str, std::ostream& os = std::cout);

			void dumpValues(u32bit n, u32bit freq, std::ostream& os = std::cout);

			void dump(std::ostream& os = std::cout);

			void dump(std::string boxName, std::ostream& os = std::cout);

#if KONDAMASK
			enum CACHE_FETCH_INFO
			{
				CACHE_FETCH_HIT = 'H',
				CACHE_FETCH_MISS = 'M',
				CACHE_FETCH_FAIL = 'F',
				CACHE_DECAY = 'D'
			};

#if KONDAMASK_CACHE_ACCESSES_CSV
#define Column(name)    name << ';'
#define ColumnEmpty()   ';'

			void LogCacheAccess(char* name, u64bit address, CACHE_FETCH_INFO fetchInfo, u32bit set, u32bit way, u64bit thisCycle, u64bit insertCycle = 0, u64bit lastHitCycle = 0, u64bit lastOnCycle = 0, bool isReplace = false)
			{
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

				osCacheAccesses <<
					Column(thisCycle) <<
					Column(name) <<
					Column(address) <<
					Column((char)fetchInfo) <<
					Column(set) <<
					Column(way);

				if (fetchInfo == CACHE_FETCH_HIT)
				{
					osCacheAccesses <<
						Column(thisCycle - insertCycle) <<
						Column(thisCycle - lastHitCycle) <<
						ColumnEmpty() <<
						ColumnEmpty();
				}
				else if (fetchInfo == CACHE_DECAY  || (fetchInfo == CACHE_FETCH_MISS && isReplace))
				{
					osCacheAccesses <<
						ColumnEmpty() <<
						ColumnEmpty() <<
						Column(thisCycle - lastHitCycle) <<
						ColumnEmpty();
				}
				else
				{
					osCacheAccesses <<
						ColumnEmpty() <<
						ColumnEmpty() <<
						ColumnEmpty() <<
						Column(thisCycle - lastOnCycle);
				}
				osCacheAccesses << std::endl;
			}

			void InitializeCacheAccessesCSV()
			{
				osCacheAccesses.open("out/CacheAccesses.csv", std::ofstream::out | std::ofstream::trunc);
				if (!osCacheAccesses.is_open())
				{
					printf("Couldn't open out/CacheAccesses.csv.\n");
					return;
				}
				osCacheAccesses << "Cycle;Cache Unit;Address;Hit/Miss;Set;Way;InsertToHitCycles;HitToHitCycles;CacheIdleCycles;CacheOffCycles" << std::endl;
			}

			void SaveCacheAccessesFile()
			{
				if (osCacheAccesses.is_open())
				{
					osCacheAccesses.close();
				}
			}
#else
			void LogCacheAccess(char* name, u64bit address, CACHE_FETCH_INFO fetchInfo, u32bit set, u32bit way, u64bit thisCycle, u64bit insertCycle = 0, u64bit lastHitCycle = 0, u64bit lastOnCycle = 0, bool isReplace = false)
			{
				return;
			}

			void InitializeCacheAccessesCSV()
			{
				return;
			}
			
			void SaveCacheAccessesFile()
			{
				return;
			}
#endif
#endif

			void finish();

		};


	} // namespace GPUStatistics

} // namespace gpu3d

#endif // STATISTICSMANAGER_H
