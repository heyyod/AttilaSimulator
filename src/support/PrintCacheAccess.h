/*
Kostas Damaskinos - 2021
*/

#ifndef PRINTCACHEACCESS
#define PRINTCACHEACCESS

#include <fstream>

static std::ofstream cacheAccessesOut;

#define CacheName(name) (std::string(name).c_str() << ';')
#define CacheType(type) (#type << ';')
#define CacheAddress(address) (address << ';')
#define CacheHit(isHit) ((isHit ? "Hit" : "Miss") << ';')

static void CacheAccessesFileNewEntry(char *name, char *type, u32bit address, bool isHit)
{
	cacheAccessesOut.open("out/CacheAccesses.csv", std::ofstream::out | std::ofstream::app);
	cacheAccessesOut <<
		name << ';' <<
		type << ';' <<
		address << ';' <<
		(isHit ? "Hit" : "Miss") <<
		std::endl;
	cacheAccessesOut.close();
}


#endif // !PRINTCACHEACCESS
