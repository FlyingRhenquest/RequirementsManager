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

#include <fr/RequirementsManager/Node.h>

namespace fr::RequirementsManager {

  /**
   * This is a Graph Node, which the User Interface will use to
   * find entire graphs. You don't have to use a graph node, but
   * if you don't, you probably won't be able to find your graph
   * in the user interface.
   *
   * * Any given graph should only have one graph node. They can
   *   have more, but then the UI will find them twice.
   * * Graph node should have a down link a node on your graph.
   *   It kinda doesn't matter where, because the whole graph
   *   will be loaded from any given node.
   *
   * * Your graph can have an up link to the graph node, but it
   *   doesn't necessarily need one if you save from the graph
   *   node. The UI will always do this, but if you're working
   *   from the lower level APIs and plan to just save the entire
   *   graph from some random node, you should make sure you have
   *   an up-link back into your graph node.
   *
   * * You can use a graph node to separate your graph from content
   *   you don't always want to load with your graph. You could
   *   have an organization have a down-link to a graph node while
   *   the graph node does not have an up-link back to the organization.
   *   When you save the organization, all its graphs will be saved,
   *   but when you save the graph only stuff in its links lists
   *   will be saved.
   *
   * * You can give the graph a title, which the UI will display
   *   when presenting nodes you can load.
   *
   * * The graph node is not particularly special as a node, it's
   *   only special to the UI.
   */

  class GraphNode : public Node {

    std::string _title;

  public:

    using Type = GraphNode;
    using Parent = Node;
    using PtrType = std::shared_ptr<Type>;

    GraphNode() = default;
    virtual ~GraphNode() = default;

    std::string getNodeType() const override {
      return "GraphNode";
    }

    void setTitle(const std::string& title) {
      _title = title;
    }

    std::string getTitle() const {
      return _title;
    }

    template <class Archive>
    void save(Archive& ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("title", _title));
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_title);
    }
  };
  
}

CEREAL_REGISTER_TYPE(fr::RequirementsManager::GraphNode);
