#include "WakaTimeHelpers.h"
#include <string>

bool FWakaTimeHelpers::PathExists (const std::string& Path) {
	struct stat Buffer;   
	return (stat (Path.c_str(), &Buffer) == 0); 
}
