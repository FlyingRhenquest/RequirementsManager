# Requirements Manager

This is a RequirementsManager because I need a Requirements Manager to
manage my requirements for this project.

Currently this is just a checkpoint commit into git. This is an ongoing
project and will change over time. It's starting to get somewhat
interesting.

This checkpoint adds database load/save support to the Python API.
See examples/python/LoadSaveDb.py for a brief example of how to use 
it.

For database support to work, you need to set up your database. If you
have a PostgreSQL database you can connect to, you can run the
CreateTables executable that builds with this project to create the
node tables. I will document this in more detail shortly.

## What's here RIGHT NOW

 * Nodes (Data objects. See Design Overview)
 * Python API for Nodes
 * Javascript API for Nodes
 * Support for writing graphs to a PostgreSQL database
 * Support for reading graphs from a PostgreSQL database
 * Python API for loading and saving nodes.
 * A VERY basic Pistache REST service for loading and saving
   nodes. The REST service can currently be started via
   the Python API, but building an executable launcher
   for it will be trivial.
 * Emscripten factory code to load nodes through REST.
 * Native factory code to load nodes through REST.
 
 Nodes are just data. They can be fit together in any way, but there
 is almost nothing right now that actually does so. You can just stick
 them together however you want to. Once I get done defining the data
 that this entire system wants to track (And there are WAY MORE of
 them than I imagined) we can actually start doing more interesting
 things with them. Because Nodes can do literally anything. That's
 why everyone keeps doing these node-based systems.
 
 PostgreSQL support is nominally ready. It has not been thoroughly tested.
 Python is nominally up to date with the C++ API. Emscripten is not
 getting SQL database support since it's not supposed to exist in
 an environment that has access to it. 
 
 UI is done through my [ImguiWidgets](https://github.com/FlyingRhenquest/ImguiWidgets) repo.
 It supports native and Emscripten builds, so you can run the UI locally
 or in your browser. Front end is not my forte, so it kinda looks
 like ass. It does work though!

## Goals:

 * Manipulate Requirements and associated stuff from C++, Python and REST
 * Traceability to the point that I can examine a requirement and
   retrieve all source code changes and unit tests associated with that
   requirement.
 * Store results of individual test runs
 * Have some things like Requirements be Commitable, which prevents them
   from changing further, while also allowing for a mechanism by which
   they *can* be changed, but the changes can be traced all the way back
   to the original document. (TODO: I plan to revisit this functionality
   and may revert everything back to just nodes -- the whole commitable
   node thing is awkward and feels like YAGNI. If I do actually need it
   I think a different approach is in order.)
 * System must be extremely flexible but also offer useful functionality
   right out of the box without having to do a huge amount of customization.
 * Must be able to store entities in this system in a SQL database.
 * GNU Affero GPL Licensed. TLDR You won't get GPL goo on your project
   data if you use this code to track your requirements, but if you
   distribute a modified version of this code or use a modified
   version of it in a SAAS application, you must make your changes
   available to download under the terms of the license.

## Design Overview:

To that end, I'm designing this as a node based system, where all
nodes have an "up" and "down" list. The up list indicates
relationships like owners, parents and that sort of
thing, the down list indicates relationships like children, owned
things -- just sort of generic "That's a that sort of relationship"
relationship. Some nodes might have other node-holders as well --
Commitable nodes like Requirements, Stories, Use Cases, etc have parent
and child nodes to track changes. So if you want to change a committed
requirement, create a new requirement awnd stick it in the child
of the committed node and eventually the system logic will know how
to provide diffs for that.

Each Node has a unique ID -- a UUIDV7 ID that is assigned when
you call the node's init() method. Currently this is done manually
but I'd like to have the system cal init for you when it makes sense
to do so. I'm just not sure that I want to do that every time an
object is created, at the moment.

Since this is basically all just nodes and we're doing a lot
of stuff in Python or via REST, this code smells a lot more like Java
than a lot of the C++ I wrote. So if you're wondering why all the
getters and setters are in there, that's why. It's very easy to just
create and assemble nodes dynamically in Python, while attempting to
do so in C++ would require compiling a program. I still want the
underlying mechanisms to be fast and accessible to external sources
from the same memory space, so it has to be in a language like
C++. And I happen to *like* writing C++ code.

## Thread Safety

I'm starting to add thread safety now, since the database stuff kind of
needs it.

## Todos

 * Docker images for the entire system so you can just run this in
   docker and play with it.
 * Logic/Rules for how nodes fit together. Right now you can just throw
   any node into any other node.
 * Node has a "changed" flag, set that for changes that need to be saved
   and figure out what sets it to false -- is it serializing to json? Saving
   in a SQL database? Other?
 * Need some nodes for tracking git commit tags. Ideally I could
   grab tags in a pre-commit hook, generate a node with that information
   and write it into the database.

## Additional notes

 * I can really store any tree-ish data this way. Json, XML, documents
   (DOM is really all just nodes.) For being so simple, nodes are pretty
   freaking useful.
 * I *do* need a new ResumeTron, and it would really take *very little* work
   to throw in the node types that I need to track jobs and skills. Don't
   be surprised if something like that emerges early in this project.

## Idioms

### C++ Code

 * It is important that all Node things be trivially constructable.
   I'm basically using default constructors for everything. Once a
   thing is defined, you can serialize it and it can exist forever.
   It has a unique ID to certify that the one you have now is the same
   exact one you had yesterday. The trivial construction is required
   to support the serialization. Access to the embodied data should be
   primarily from the UI. Nodes are essentially just plain old data
   objects.
 * Most node types will have Type, PtrType and Parent defined. This
   lets me reparent a node type fairly easily, change the pointer type
   from shared pointer to something else very easily if I need to and
   make the cereal archiver functions more generic.
 * Most nodes have getters and setters for their specific data. This
   smells like Java but is necessary due to exposing the objects via
   the Python API.
 * I'm trying to keep node-specific data on the lean side, favoring
   composing specific conceptual objects (in ISO or whatever) using
   several node types when possible. This makes it possible to add
   stakeholders to any node type a stakeholder might be interested in
   and add an email address and phone number to a stakeholder or
   any other thing that needs an email address or a phone number.
 * CommitableNodes can be committed and then you have to change them
   using an official process. Basically anytime you inherit something
   from CommitableNode, drop a "throwIfCommitted()" call in any setter
   that should not allow a change to be made if the node is committed.
   This lets you focus on your logic without having to worry too
   much about what happens if the object does happen to be committed.
 * to_json will "just work" with most nodes, as it leverages the
   cereal archiver to generate the JSON. You can build an entire
   graph, call to_json on any of its nodes and the entire graph
   will be serialized.
