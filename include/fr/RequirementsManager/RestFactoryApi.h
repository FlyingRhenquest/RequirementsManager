/**
 * Copyright 2026 Bruce Ide
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.

 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <fr/RequirementsManager/Node.h>
#include <fr/RequirementsManager/GraphNode.h>
#include <fr/RequirementsManager/ServerLocatorNode.h>
#include <fteng/signals.hpp>
#include <memory>

// Factory APIs for various nodes that can be served over REST.
// At the moment this is Graph Nodes and Server Locator Nodes.
// These just define the base APIs to implement a factory to.

// Due to some emscripten-related ass-pain (That might also
// be true in native) the emscripten versions of these factories
// are singletons, so I can't just declare shared pointers
// to these as I would normally do. So these objects
// effectively serve as null objects and don't do anything
// if they're not implemented.

namespace fr::RequirementsManager {

  class ServerLocatorNodeFactory {
  public:
    // Available signal is called whenever a node has been deserialized and
    // is now available. You subscribe to this to get nodes
    fteng::signal<void(std::shared_ptr<ServerLocatorNode>)> available;
    // Error is called with string (Maybe even a populated one!) if an
    // error occurs. If this happens, this factory might be broken
    // and never work.
    fteng::signal<void(const std::string &)> error;

    ServerLocatorNodeFactory() {};
    virtual ~ServerLocatorNodeFactory() {};

    virtual void fetch(const std::string& url) {};
  };

  class GraphNodeFactory {
  public:    
    // Available signal is called whenever a node has been deserialized and
    // is now available.
    fteng::signal<void(std::shared_ptr<Node>)> available;
    // Error is called with a string if an error occurs
    fteng::signal<void(const std::string&)> error;

    GraphNodeFactory() {}
    virtual ~GraphNodeFactory() {}

    // Fetch a URL (Subscribe to callbacks before running this)
    virtual void fetch(const std::string& url) {};
    // POST a node back to the REST server
    virtual void post(std::string url, std::shared_ptr<Node> node) {}
  };
  
}
