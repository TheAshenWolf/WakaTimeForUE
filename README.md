# WakaTimeForUE4

Make sure you have Python, pip and wakatime installed and **available from cmd/terminal**.
That means both installing it and adding it into your PATH variable.

### Installing Python
You can get python here: https://www.python.org/downloads/

1. Run the downloaded .exe file
2. Tick "Add Python ... to PATH" at the bottom of the window.
3. Click **Install Now**
4. Open cmd/terminal
5. Use command **pip install wakatime**
6. Done.

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
1. When you first run the plugin, you get prompted with a window.
2. Paste in your API key
3. Select whether you are a developer or a designer. This will determine if your HeartBeats are sent as "Coding" or "Designing"
4. Save.
5. If you want to change the Settings, the button can be found in the toolbar.


### Notice
This is my first ever project in C++, so it is definitely not perfect.  
If you have any suggestions how to improve it, or any bug reports, please, use the "Issues" tab.
