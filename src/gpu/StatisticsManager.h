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

			void finish();

			// KONDAMASK_CACHE_LOG_CSV
			std::ofstream osCacheAccesses;

			enum CACHE_LOG_INFO
			{
				CACHE_FETCH_HIT,
				CACHE_FETCH_MISS,
				CACHE_FETCH_FAIL,
				CACHE_DECAY,
				CACHE_DECAY_RESERVED,
				CACHE_DECAY_REPLACING,
				CACHE_WRITE_REQUEST,
				CACHE_READ_REQUEST,
				CACHE_WRITE_TRANS,
				CACHE_READ_TRANS,
				CACHE_DECAY_FLUSH_FAILED
			};

			void LogCacheAccess(char* name, u64bit address,
				CACHE_LOG_INFO logInfo,
				u32bit set, u32bit way, u64bit thisCycle,
				u64bit insertCycle = 0,
				u64bit lastHitCycle = 0,
				u64bit lastOnCycle = 0,
				bool isReplace = false)
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

			void InitializeCacheAccessesCSV()
			{
				if (!KONDAMASK_CACHE_LOG_CSV)
					return;

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
				if (!KONDAMASK_CACHE_LOG_CSV)
					return;

				if (osCacheAccesses.is_open())
				{
					osCacheAccesses.close();
				}
			}
		};


	} // namespace GPUStatistics

} // namespace gpu3d

#endif // STATISTICSMANAGER_H
