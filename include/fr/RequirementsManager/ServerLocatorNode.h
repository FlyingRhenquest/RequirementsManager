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

namespace fr::RequirementsManager {

  /**
   * This node holds some key pieces of data that need to get
   * transferred between the rest server in GraphServer.h
   * and the client UI. These are:
   *
   * * The UUID of an individual graph
   * * The title of an individual graph
   * * The server address to query the server to retrieve that graph.
   *
   * The server will return a JSON-serialized vector of these nodes
   * to the client when the client makes a request.
   *
   * This data is actually const and only settable at
   * ServerLocatorNode construction time.
   */

  class ServerLocatorNode : public Node {
    std::string _graphUuid;
    std::string _graphTitle;
    std::string _graphAddress;

  public:
    using Type = ServerLocatorNode;
    using Parent = Node;
    using PtrType = std::shared_ptr<Type>;
    
    ServerLocatorNode(const std::string& graphUuid,
                      const std::string& graphTitle,
                      const std::string& graphAddress) :
      _graphUuid(graphUuid),
      _graphTitle(graphTitle),
      _graphAddress(graphAddress) {
    }

    ServerLocatorNode() {}

    ServerLocatorNode(const ServerLocatorNode& copy) :
      _graphUuid(copy._graphUuid),
      _graphTitle(copy._graphTitle),
      _graphAddress(copy._graphAddress) {
    }

    ServerLocatorNode(ServerLocatorNode&& move) :
      _graphUuid(std::move(move._graphUuid)),
      _graphTitle(std::move(move._graphTitle)),
      _graphAddress(std::move(move._graphAddress)) {
    }
    
    virtual ~ServerLocatorNode() = default;

    std::string getNodeType() const override {
      return "ServerLocatorNode";
    }
    
    std::string getGraphUuid() {
      return _graphUuid;     
    }

    std::string getGraphTitle() {
      return _graphTitle;
    }

    std::string getGraphAddress() {
      return _graphAddress;
    }

    template <class Archive>
    void save(Archive& ar) const {
      ar(cereal::make_nvp(Parent::getNodeType(), cereal::base_class<Parent>(this)));
      ar(cereal::make_nvp("graphUuid", _graphUuid));
      ar(cereal::make_nvp("graphTitle", _graphTitle));
      ar(cereal::make_nvp("graphAddress", _graphAddress));
    }

    template <class Archive>
    void load(Archive &ar) {
      ar(cereal::base_class<Parent>(this));
      ar(_graphUuid);
      ar(_graphTitle);
      ar(_graphAddress);
    }
    
  };
  
}
