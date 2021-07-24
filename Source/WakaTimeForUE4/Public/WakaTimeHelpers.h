#pragma once

#include <string>

class FWakaTimeHelpers
{
public:
	/// <summary>
	/// Checks whether a file or directory on given path exists
	/// </summary>
	/// <param name="Path"> Path to the location </param>
	/// <returns> true if file or directory exists, false otherwise </returns>
	/// <remarks> According to StackOverflow - PherricOxide, this is the fastest method to check </remarks>
	static bool PathExists(const std::string& Path);
};
