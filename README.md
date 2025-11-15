# Requirements Manager

This is a RequirementsManager because I need a Requirements Manager to
manage my requirements for this project.

Currently this is just a checkpoint commit into git. This is an ongoing
project and will change over time. It's not particularly interesting
right now.

The Python API is up-to-date with the C++ objects, so I could start
putting requirements together in Python, but I'm not gonna do that yet.

## What's here RIGHT NOW

 * Nodes (Data objects. See Design Overview)
 * Python API for Nodes
 * Javascript API for Nodes
 
 Nodes are just data. They can be fit together in any way, but there
 is almost nothing right now that actually does so. You can just stick
 them together however you want to. Once I get done defining the data
 that this entire system wants to track (And there are WAY MORE of
 them than I imagined) we can actually start doing more interesting
 things with them. Because Nodes can do literally anything. That's
 why everyone keeps doing these node-based systems.

## Goals:

 * Manipulate Requirements and associated stuff from C++, Python and REST
 * Traceability to the point that I can examine a requirement and
   retrieve all source code changes and unit tests associated with that
   requirement.
 * Store results of individual test runs
 * Have some things like Requirements be Commitable, which prevents them
   from changing further, while also allowing for a mechanism by which
   they *can* be changed, but the changes can be traced all the way back
   to the original document.
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

Since this is basically all just nodes and we're planning to do
a lot of stuff in Python or via REST (and eventually something like
React,) this code smells a lot more like Java than a lot of the C++
I wrote. So if you're wondering why all the getters and setters
are in there, that's why. It's very easy to just create and assemble
nodes dynamically in Python, while attempting to do so in C++
would require compiling a program. I still want the underlying 
mechanisms to be fast and accessible to external sources from the
same memory space, so it has to be in a language like C++. And I
happen to *like* writing C++ code.

## Thread Safety

Will eventually need some, don't have any yet. It is on the radar though.
This may involve doing stuff like hiding up and down lists behind 
additional accessors (yay) so I can do mutex locking in sensible
places.

## Todos

 * Do an example project with this requirements manager and export it to
   JSON that anyone can just import so we can all manage changes to the
   requirements manager from the requirements manager.
 * SQL logic to save nodes (Probalby some visitor-like thing initially
   geared toward PostgresSQL)
 * Logic/Rules for how nodes fit together. Right now you can just throw
   any node into any other node.
 * Node has a "changed" flag, set that for changes that need to be saved
   and figure out what sets it to false -- is it serializing to json? Saving
   in a SQL database? Other?
 * Figure out a way to export one chunk of the project -- one requirement,
   all changed things, something like that. I'm thinking I can set
   a filter in the cereal serialization and have it just stop following
   up nodes when it hits a certain point. Cereal is already pretty good
   of not recursively exploding when you serialize a whole graph of
   things from any individual node in it.
 * Need some nodes for tracking git commit tags. Ideally I could
   grab tags in a pre-commit hook, generate a node with that information
   and write it into the database.

## Additional notes

 * All these nodes together make a graph. I don't think I need an explicit
   graph object to store them, at least not yet.
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
   cereal archiver to generate the JSON.
