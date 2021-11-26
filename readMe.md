# Terminal Based Linux FileExplorer
## Prerequisites
**Platform:** Linux <br/>
**Software Requirement**
1. G++ compiler
   * **To install G++ :** ```sudo apt-get install g++```

## To run project
```
g++ main.cpp
./a.out
```
## Getting started
1. Default mode is normal mode and press ":" to switch to command mode.
2. Press ESC key to go back to normal mode from command mode.
3. Press q key from normal mode to exit project.
4. In command mode backspace key is used to erase text.

## Normal Mode
* List of directories and files in the current folder are displayed with the following attributes in human readable format:
   * Type (file or directory)
   * File Name
   * File Size
   * Ownership (user and group)
   * Permissions
   * Last modified
* Shows entries “.” and “..” for current and parent directory respectively.
* Use up and down arrow keys to scroll through entries.
* Use K and I keys for page scrolling.
*  Use enter key to open file (in VI editor) / directory.
* Traversal
   * Go back - Left arrow key takes the user to the previously visited directory.
   * Go forward - Right arrow key takes the user to the next directory
   * Up one level - Backspace key takes the user up one level.
   * Home - ​ h ​ key

## Command Mode
* The application enters the Command Mode whenever ```:``` key is pressed. In the command mode, the user should be able to enter different commands. All commands appear in the status bar at the bottom.
* List of commands:
   1. ```copy <source_file(s)> <destination_directory> ```
   2. ```move <source_file(s)> <destination_directory>```
   3. ```rename <old_file_path> <new_file_path>```
   4. ```create_file <file_name> <destination_path>```
   5. ```create_dir <dir_name> <destination_path>```
   6. ```delete_file <file_path>```
   7. ```delete_dir <dir_path>```
   8. ```goto <location>```
   9. ```search <file_name>``` or ```search <directory_name>```
* On pressing ​ ESC​ key, the application goes back to the Normal Mode.
