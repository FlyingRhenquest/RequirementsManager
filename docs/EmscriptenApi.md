# Some instructions for compiling with emscripten

## Install Emscripten on Linux

Sorry Windows users, you're on your own. I don't hate you, I just
don't have a Windows box to test this.

You'll want enscripten, nodejs and node

sudo apt install enscripten nodejs npm

You'll also want cmake if you don't already have it.

##

Build for wasm:

    mkdir -p /tmp/wasm
    cd /tmp/wasm
    emcmake cmake ~/sandbox/RequirementsManager (Or wherever you cloned the rep to)
    cmake --build .
	
This will download and build the boost bits it needs, and cereal. Even
if you have boost already installed on your system, we need to build
webassembly versions of the libraries. Welcome to crosscompiling things.

If that compiles without errors, see if you can run a javascript script
to use the library:

    cp ~/sandbox/RequirementsManager/examples/javascript/NodeToJson.js .
	node NodeToJson.js

You should receive output similar to:


    {
        "Node": {
            "id": "019a8466-2528-7000-8025-ca48cf0c16c9", 
			"upList": [],
			"downList": []
		}
	}
	
The unique ID will change each time since we're calling Node::init
and that's creating a UUID V7 ID with boost::uuid.

