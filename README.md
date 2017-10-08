# nextspider
a multi-thread web spider written in C and extened by Lua

## Install
Please download at <https://github.com/qianngchn/nextspider/releases>.

Then modify configuration file `~/.nextspider/config.lua`(Linux) or `config.lua`(Windows).

A simple example:

    -- script: default lua script executed by nextspider, script should be located in the relative path of this folder
    -- if you want to execute a lua script which is in the absolute path, you can use -s option in the command line
    script = "default.lua"
    -- cookies: with cookies nextspider can access url with logging and more features
    --cookies = "username=qianngchn; password=0123456789;"
    -- proxy: proxy setting used by nextspider
    --proxy = "http://127.0.0.1:7777"
    -- check_mode: check whether lua script executes correctly, without downloading any file
    check_mode = false
    -- quiet: whether to write output to stdout
    quiet = false
    -- logger: whether to write output to stat.log in this folder
    logger = false

Command line options can override options in the configuration file.

## Lua Scripts
nextspider uses lua scripts to control download hooks, extend functions and adapt to websites.

A default lua script is like this:

    -- url: the value is the url passed to the lua script by nextspider
    -- html: the value is the html content of url passed to the lua script by nextspider
    -- nexturl: the value is the next url to access and download for parsing html, will be returned to nextspider
    -- files: the value is the files to download, will be returned to nextspider
    -- stop: the value determines whether nextspider continues to access url and download files

    function parse_html(url, html)
        local nexturl = "https://github.com/qianngchn/nextspider"
        local files = "https://raw.githubusercontent.com/qianngchn/nextspider/master/config.lua config.lua https://raw.githubusercontent.com/qianngchn/nextspider/master/default.lua default.lua"
        local stop = true
        return nexturl, files, stop
    end

Optionally `nextspider -u` will show lua scripts of mine, you can choose to download as you like.

## Command Line
You can get help information with `nextspider -h`, here is the usage:

    Usage: nextspider [-s luascript] [-k cookies] [-p proxy] [-u] [-c] [-q] [-l] [-h] <url> <url> ...
      -s: which lua script to execute
      -k: cookies used to access url with logging and more features
      -p: support any proxy, include a HTTP proxy tunnel, example: "http://127.0.0.1:7777"
      -u: get the latest release and more lua scripts
      -c: check mode for testing lua script, whithout downloading any files
      -q: quiet mode, do not write output to stdout
      -l: logger mode, write output to log file
      -h: show help information

## Build
To build, you should install `liblua` and `libcurl` for developers first.

* **Linux:** run the following commands

        git clone https://github.com/qianngchn/nextspider.git
        cd nextspider
        make linux

* **Windows:** for gcc you need mingw, cygwin or other tools, then you can run following commands

        git clone https://github.com/qianngchn/nextspider.git
        cd nextspider
        make mingw
