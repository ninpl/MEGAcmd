# MEGAcmd - Command Line Interactive and Scriptable Application
MEGAcmd provides non UI access to MEGA services. It intends to offer all the
functionality with your MEGA account via commands. It features **synchronization**
, **backup** of local folders into your MEGA account and a **webdav/streaming** server.

See [`Usage Examples`](#usage-examples).

Available packages for MEGAcmd in all supported platforms should be found
[here](https://mega.nz/cmd). If the package fails to install, read below for the requirements.

**It supports 2 modes of interaction:**

- **Interactive** - A shell to query your actions
- **Scriptable** - A way to execute commands from a shell/a script/another program

In order to provide those 2 modes, it features one server (**MEGAcmdServer**), an interactive shell (**MEGAcmdShell**) and several commands that will launch the non-interactive client (**MEGAcmdClient**).

See [`Usage`](#usage) and [`Platform`](#platforms) to understand how to use it in your particular system.

# Building MEGAcmd
## Requirements
All dependencies are downloaded and installed automatically by vcpkg while building. The local path to the vcpkg repository can be set with the `-DVCPKG_ROOT` flag (defaults to `../vcpkg` if the flag is missing).

## Getting the source
Ensure you obtain the repository recursively.
```
git clone https://github.com/meganz/MEGAcmd.git
cd MEGAcmd && git submodule update --init --recursive
```

## Building
To build MEGAcmd, we first need to configure the project using CMake:
```
cmake -B build/build-cmake-Debug -DCMAKE_BUILD_TYPE=Debug
```
A different build type can be selected out of `Debug`, `Release`, `MinSizeRel`, or `RelWithDebInfo`. By convention, we use `build/build-cmake-[Debug|Release|...]` as build directory, but any other path can be selected with the `-B` option.

To build unit and integration tests as well, we can add the `-DENABLE_MEGACMD_TESTS=ON` flag.

If `ccache` is installed in our system, we can use it to speed up subsequent compilations by adding the `-DCMAKE_CXX_COMPILER_LAUNCHER=ccache` flag.

After the project has been configured, we can build it with:
```
cmake --build build/build-cmake-Debug
```
To speed up compilation, we can use the `-j` option to specify a number of cores.

## System-wide installation (Linux/macOS)
On Unix-based systems, such as Linux or macOS, MEGAcmd can be installed with (after building):
```
sudo cmake --install build/build-cmake-Release
```
This will install `mega-cmd`, `mega-cmd-server`, and all `mega-*` commands such as `mega-put`, `mega-cd`, etc. It is recommended to avoid installing a Debug build, since some paths might not be properly setup.

We can specify the default directory to install from by adding the `-DCMAKE_INSTALL_PREFIX` flag while configuring.

# Usage
Before explaining the two ways of interaction, it's important to understand how MEGAcmd works. When you login with MEGAcmd, your session, the list of synced folders, cache databases, and extra configuration are stored in your local home folder. Closing it does not delete those, and restarting the MEGAcmd server will restore your previous session (similary to the MEGA Desktop App, which won't ask for user/password once restarted).

You will need to `logout` properly in order to clean your data.

Now let's get into details of the two usage modes. Both modes require that MEGAcmdServer is running. You can manually launch it. Fortunately, you can also open the interactive shell or execute any command and the server will start automatically.

## Interactively:
Execute MEGAcmd shell. [`Platform`](#platforms) section explains how to do that in the different supported systems. You should be facing an interactive shell where you can start typing your commands, with their arguments and flags.

You can list all the available commands with `help`.

And obtain useful information about a command with `command --help`

First you would like to log in into your account. Again, notice that doing this stores the session and other stuff in your home folder. A complete logout is required if you want to end you session permanently and clean any traces (see `logout --help` for further info).

## Non-interactively:
When MEGAcmd server is running, it will be listening for client commands. Use the different `mega-*` commands available.

`mega-help` will list all these commands (you will need to prepend "mega-" to the commands listed there). To obtain further info use `mega-command --help`.

Those commands will have an output value != 0 in case of failure.
See [megacmd.h](https://github.com/meganz/MEGAcmd/blob/master/src/megacmd.h) to view the existing error codes.

Ideally, you would like to have these commands in your `PATH` variable
(See [`Platform`](#platforms) for more info). For further info use `mega-help --non-interactive`.

## Usage examples
Here are some examples of use (more info and usage examples are available at the [User Guide](UserGuide.md)).

**Notice:** the commands listed here assume you are using the interactive interaction mode: they are supposed to be executed within MEGAcmdShell.


* A **synchronization** can be established simply by typing:
```
sync /path/to/local/folder /folder/in/mega
```
This will synchronize the contents in your local and your mega folder both ways.

* You can also set remote **backups** of a local folder to keep historical snapshots of your files. So simple as:
```
backup /path/mega/folder /remote/path --period="0 0 4 * * *" --num-backups=10
```
This will configure a backup of "myfolder" into /remote/path that will be carried out at 4:00 A.M. (UTC) every day. It will store the last 10 copies.

 Further info on backups [here](contrib/docs/BACKUPS.md).

* You **serve a location** in your MEGA account via WebDAV:
```
webdav /path/mega/folder
```

* Or **stream a file** in your MEGA account:
```
webdav /path/to/myfile.mp4
```
Further info on webdav and streaming [here](contrib/docs/WEBDAV.md).

* **Download the contents** of a shared link:
```
get https://mega.nz/#F!ABcD1E2F!gHiJ23k-LMno45PqrSTUvw /path/to/local/folder
```

Now let's do something more complicated with non-interactive usage using some GNU tools (similar stuff can be easily done in Windows as well):

* We want to provide something crypto secured with only 10 minutes of access:
```
mega-put /path/to/my/temporary_resource /exportedstuff/
mega-export -a  /exportedstuff/temporary_resource --expire=10M | awk '{print $4}'
```

* Or imagine we'd like to public the enterprise promotional videos of May 2015 that we have previously stored in MEGA:
```
for i in $(mega-find /enterprise/video/promotional2015/may --pattern="*mpeg"); do
mega-export -a $i | awk '{print $4}';
done
```

# Platforms
## Linux
If you have installed MEGAcmd using one of the available packages at [here](https://mega.nz/cmd).

Or have it built without `--prefix`, both the server (`mega-cmd-server`), the shell (`mega-cmd`) and the different client commands (`mega-*`) will be in your `PATH` (on a fresh install, you might need to open your terminal again).

If you are using bash, you should also have autocompletion for client commands working.

If that is not you case, include the location for the binaries in your `PATH` variable.


## Windows
You can have MEGAcmd installed using the installer from [here](https://mega.nz/cmd).

If you are interested in installing MEGAcmd without human intervention, notice that this installer supports silent installation, you just need to execute in your command prompt:
```
MEGAcmdSetup.exe /S
```

Once you have MEGAcmd installed, you just need to execute it (via Desktop icon or Start Menu) to open the shell.  This will open MEGAcmdServer in the background (a process named MEGAcmdServer.exe).

For a better user experience (specially in Windows 7) we recommend executing MEGAcmd from PowerShell.

**Open PowerShell and execute:**

```
$env:PATH += ";$env:LOCALAPPDATA\MEGAcmd"
MEGAcmdShell
```

For *non-interactive* usage, there are several `mega-*.bat`  client commands you can
use writting their absolute paths, or including their location into your environment `PATH`
 and execute them normally (`mega-*`).

If you use PowerShell and you have installed the official MEGAcmd, you can do that simply with:

```
$env:PATH += ";$env:LOCALAPPDATA\MEGAcmd"
```

Client commands completion requires bash, hence, it is not available for Windows.

### Caveats
Although there have been several efforts in having non-ASCII unicode characters supported
in Windows, there still may be some issues. Pay special attention if you are willing to use pipes or
send the output of a command into a file from your client commands. See `help --unicode`
for further info regarding that.

## macOS
Install MEGAcmd from [here](https://mega.nz/cmd).

For macOS, after installing the dmg, you can launch the server using MEGAcmd in Applications. If you wish to use the client commands from macOS Terminal, open the Terminal and include the installation folder in the `PATH`.

**Typically:**

```
export PATH=/Applications/MEGAcmd.app/Contents/MacOS:$PATH
```

And for **bash completion**, source `megacmd_completion.sh`:

```
source /Applications/MEGAcmd.app/Contents/macOS/megacmd_completion.sh
```
*Note for macOS Catalina or above*: since Catalina, macOS uses `zsh` as default shell. If you want to have auto completion, we strongly recommend you to use `bash` shell (just execute `bash` in your terminal).

## NAS systems
Currently we have build scripts for **Synology**, which can be found in the `build/SynologyNAS` folder along with instructions on how to set up the build.  Typically this results in a 'package' which can then be manually installed in the NAS. To use MEGAcmd on those systems, ssh into the device and run the commands as normal (having first added their folder to your `PATH` variable).


# Features:
## Autocompletion:
MEGAcmd features autocompletion in both interactive and non-interactive (only for bash) mode. It will help completing both local and remote (Mega Cloud) files, flags for commands, values for flags/access levels, even contacts.

## Verbosity
There are two different kinds of logging messages:
- **SDK based**: those messages reported by the sdk and dependent libraries.
- **MEGAcmd based**: those messages reported by MEGAcmd itself.

You can adjust the level of logging for those kinds with `log` command.

However, passing `-v` (`-vv`, `-vvv`, and so on for a more verbose output)
to an specific command will use higher level of verbosity of MEGAcmd based messages.

Further info on verbosity [here](contrib/docs/DEBUG.md).

## Regular Expressions
If you have compiled MEGAcmd with PCRE (enabled by default), you can use PCRE compatible expressions in certain commands with the flag `--use-pcre`. Otherwise, if compiled with c++11, c++11 regular expressions will be used.

 If none of the above is the case, you can only use wildcards: "*" for any number of characters or "?" for a single unknown character.

You can check the regular expressions compatibility with `find --help`:
```
find --help
...
Options:
 --pattern=PATTERN	Pattern to match (Perl Compatible Regular Expressions)
```

**Notice:** if you use MEGAcmd in non interactive mode, notice that shell pattern will take precedence. You will need to either escape symbols like `*` (`\*`) or surround them between quotes (e.g: "*.txt").

## MEGAcmd Updates
MEGAcmd updates automatically for Windows & macOS.

For Linux, whenever there is a new update, it will be published in the corresponding repository and your system's updating tool will let you update it.

### Disable automatic updates
You can type `update --auto=OFF` to disable automatic updates. `update --auto=ON` will re-enable them.

If you want to see the state of automatic updates you can use `update --auto=query`. This will inform if automatic updates are enabled or not.

Notice that MEGAcmdServer must be running in order to have automatic updates working.

You can also update manually by typing `update` within MEGAcmd. This will check if there are updates available and proceed to update if affirmative. Whenever MEGAcmd is updated it will be restarted (all open instances of MEGAcmdShell will be restarted too).

Alternatively you can also execute `MEGAcmdUpdater.exe` in Windows or `MEGAcmdUpdater` (located at /Applications/MEGAcmd.app/Contents/MacOS) in macOS.

## Known Bugs
Currently there are certain discrepancies with **PATHS** when logging into a public folder.

For instance, imagine a folder named `toshare` with a subfolder named `x`. If we login in to `toshare` and execute `find /x`, this will be the output:

```
/toshare/x
```

Whereas if we execute `find /toshare/x`, we receive an error, since folder absolute path
refers to `/` as root path.

```
[err: 12:21:51] Couldn't find /toshare/x
```

It might better be referred as `/toshare/x`.
