/**
 * Copyright 2025 Bruce Ide
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

#include <fr/RequirementsManager/GraphNode.h>
#include <memory>
#include <pqxx/pqxx>
#include <unordered_map>

namespace fr::RequirementsManager {

  /**
   * Reads the Graph Nodes database table and stores a list of
   * titles and UUIDS. This can be displayed in my Imgui widgets
   * so the graph can be loaded in to the GUI. It could also
   * be used to implement a Pistache REST server that feeds JSON
   * to a web service.
   */

  class GraphNodeLocator {
  public:
    // Map type is indexed on uuid, with graph node title in the
    // value.
    using MapType = std::unordered_map<std::string, std::string>;
    using Type = GraphNodeLocator;
    using PtrType = std::shared_ptr<GraphNodeLocator>;

  private:
    pqxx::connection _connection;
    pqxx::work _transaction;

  public:
    MapType nodes;

    GraphNodeLocator() : _transaction(_connection) {}
    ~GraphNodeLocator() {}

    // Query queries the graph node database table and
    // loads the nodes map with data
    void query() {
      // Always load it fresh
      nodes.clear();
      std::string cmd("select id, title from graph_node");
      pqxx::result res = _transaction.exec(cmd);
      for (auto const &row : res) {
        auto id = row[0].as<std::string>();
        auto title = row[1].as<std::string>();
        nodes[id] = title;
      }
    }
    
  };
  
}
