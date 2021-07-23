#pragma once
#include <string>

class FWakaTimeHelpers
{
public:
	/// <summary>
	/// Checks whether a file on given path exists
	/// </summary>
	/// <param name="Path"> Path to the file </param>
	/// <returns> true if file exists, false otherwise </returns>
	/// <remarks> According to StackOverflow - PherricOxide, this is the fastest method to check if file exists </remarks>
	static bool FileExists(const std::string& Path);

	
};
