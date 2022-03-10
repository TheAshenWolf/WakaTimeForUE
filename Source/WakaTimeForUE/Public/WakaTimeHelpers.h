#pragma once

#include <string>

class FWakaTimeHelpers
{
public:
	/// <summary>
	/// Checks whether a file or directory on given path exists
	/// </summary>
	/// <param name="Path"> Path to the location </param>
	/// <returns> True if file or directory exists, false otherwise </returns>
	/// <remarks> According to StackOverflow - PherricOxide, this is the fastest method to check </remarks>
	static bool PathExists(const std::string& Path);

	/// <summary>
	///	Runs a command using an exe file
	/// </summary>
	/// <param name="CommandToRun"> Command that should be passed to the exe file as arguments </param>
	/// <param name="bRequireNonZeroProcess"> Also checks for the inner process exit code, requiring it to be non-zero </param>
	/// <param name="ExeToRun"> Path to the exe </param>
	/// <param name="WaitMs"> How long to wait for the process to finish </param>
	/// <param name="bRunPure"> If false, prepends "/c start /b" to the command (mainly for CMD use) </param>
	/// <param name="Directory"> Path to the directory to start the process in </param>
	/// <returns> True, if process succeeds (and its return code is non-zero if required) </returns>
	static bool RunCommand(std::string CommandToRun, bool bRequireNonZeroProcess = false,
	                       std::string ExeToRun = "C:\\Windows\\System32\\cmd.exe", int WaitMs = 0,
	                       bool bRunPure = false,
	                       std::string Directory = "");

	/// <summary>
	///	Overload for RunCommand that uses Powershell
	/// </summary>
	/// <param name="CommandToRun"> Command that should be passed to the exe file as arguments </param>
	/// <param name="bRequireNonZeroProcess"> Also checks for the inner process exit code, requiring it to be non-zero </param>
	/// <param name="WaitMs"> How long to wait for the process to finish </param>
	/// <param name="bRunPure"> If false, prepends "/c start /b" to the command (mainly for CMD use) </param>
	/// <param name="Directory"> Path to the directory to start the process in </param>
	/// <returns> True, if process succeeds (and its return code is non-zero if required) </returns>
	static bool RunPowershellCommand(std::string CommandToRun, bool bRequireNonZeroProcess = false, int WaitMs = 0,
	                                 bool bRunPure = false,
	                                 std::string Directory = "");

	/// <summary>
	///	Overload for RunCommand that uses Cmd
	/// </summary>
	/// /// <param name="CommandToRun"> Command that should be passed to the exe file as arguments </param>
	/// <param name="bRequireNonZeroProcess"> Also checks for the inner process exit code, requiring it to be non-zero </param>
	/// <param name="WaitMs"> How long to wait for the process to finish </param>
	/// <param name="bRunPure"> If false, prepends "/c start /b" to the command (mainly for CMD use) </param>
	/// <param name="Directory"> Path to the directory to start the process in </param>
	/// <returns> True, if process succeeds (and its return code is non-zero if required) </returns>
	static bool RunCmdCommand(std::string CommandToRun, bool bRequireNonZeroProcess = false, int WaitMs = 0,
	                          bool bRunPure = false,
	                          std::string Directory = "");

	/// <summary>
	/// Unzips a .zip archive into a directory using powershell
	/// </summary>
	/// <param name="ZipFile"> Path to the zip file </param>
	/// <param name="SavePath"> Directory to extract to </param>
	/// <returns> True if process succeeded </returns>
	static bool UnzipArchive(std::string ZipFile, std::string SavePath);

	/// <summary>
	/// Downloads a file into a directory using powershell
	/// </summary>
	/// <param name="URL"> Url where to download the file from </param>
	/// <param name="SaveTo"> Path where to save the file </param>
	/// <returns> True if process succeeded </returns>
	static bool DownloadFile(std::string URL, std::string SaveTo);
};
