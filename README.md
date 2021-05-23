# WakaTimeForUE4

![plugin version](https://img.shields.io/badge/version-1.1.2-blue) ![Unreal Engine version](https://img.shields.io/badge/Unreal%20Engine%20version-4.26-blue)

---

Make sure you have Python, pip and wakatime installed and **available from cmd/terminal**.
That means both installing it and adding it into your PATH variable.

### Installing Python
You can get python here: https://www.python.org/downloads/

1. Run the downloaded .exe file
2. Tick "Add Python ... to PATH" at the bottom of the window.
3. Click **Install Now**
4. Optional: Click "Disable PATH length limit"
5. Open cmd/terminal
6. Use command **pip install wakatime**
7. Done.

Check if everything works with commands  
**python --version**  
**pip -h**  
**wakatime -h**  

### Installing the plugin to a Unreal Engine project
1. Click the green button labeled "Code"
2. Click "Download ZIP"
3. Extract the folder
4. Locate the project folder
5. Go inside the folder (You should see [projectName].uproject file)
6. Create a folder "Plugins", if there is not one yet.
7. Paste the WakaTimeForUE4 folder (it contains WakaTimeForUE4.uplugin in it) in there.
8. Enjoy! The plugin can now be used in your Unreal Engine 4 Project.

### Setting up the plugin
1. When you first run the plugin, you get prompted with a window (if you have already used the plugin in a different project, your data should be kept saved.)
    - You can open these settings from within the toolbar
2. Paste in your API key
3. Select whether you are a developer or a designer. This will determine if your HeartBeats are sent as "Coding" or "Designing"
4. Save.
5. If you want to change the Settings, the button can be found in the toolbar.

### Notice
This is my first ever project in C++, so it is definitely not perfect.  
If you have any suggestions how to improve it, or any bug reports, please, use the "Issues" tab.

### Troubleshooting
Q: **The Wakatime icon in my editor is pink, what is wrong?**  
A: Your plugin folder is most likely named something like `WakaTimeForUE4-1.1.0`. Rename it to just `WakaTimeForUE4` and it will be fine.

Q: **When saving a project, the plugin throws an error "Windows cannot find 'wakatime'. Make sure you've typed the name correctly, then try again.". What to do?**  
A: First of all, check all the commands written above; If they function within the cmd, but not inside the unreal engine, try restarting your computer.

Q: **The plugin failed to build.**  
A: The branch `main` is a development branch, so it might contain code that does not compile. If this happens on the `release` branch (or any version tag), make sure you have all dependencies required for building C++ code. You usually install all of them when installing Visual Studio and checking both C++ and .NET.
