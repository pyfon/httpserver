# C++ HTTP Server
This is a basic HTTP/Web server written in pure C++17.  
This program was written as a practise/fun exercise, it is **not** for production use!


## Build
After you've cloned this repo:  
1. Install needed libraries:  
`cd httpserver`
2. Build the project:  
`make`
3. Run:  
`sudo bin/webserver`

## Usage
The available options can be seen with --help:  

    Nathan's Webserver
        Options:
        -d, --directory <dir>
            Serve content from <dir> (default ".")
        -h, --help
            Print this help message and exit
        -i, --index-document <file>
            Use <file> as default document (default "index.html")
