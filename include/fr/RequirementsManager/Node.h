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
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/xml.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>


namespace fr::RequirementsManager {

  /**
   * A node in a Requirements graph. Many entities can be nodes,
   * this specifies the basic API of all those entities
   */

  struct Node : public std::enable_shared_from_this<Node> {
    using PtrType = std::shared_ptr<Node>;

    // Lock nodeMutex before changing values, serializing/deserializing
    // or storing the node in a database. Always release the
    // mutex as soon as possible after locking it.
    mutable std::mutex nodeMutex;
    // Use up for things like parent(s), required-by, owner(s), etc
    std::vector<PtrType> up;
    // Use down for things like children, requires, owned things, etc
    std::vector<PtrType> down;
    // All nodes may have an assigned UUID. This UUID is declared
    // in this class but is not populated upon creation of the object,
    // as I don't necessarily want to pay the overhead for one every
    // time I create a node. You need boost 1.86 or later for
    // UUID V7 UUIDs (time_generator_7)
    boost::uuids::uuid id;
    // Track that information in this node changed. This could
    // be caused by adding something to an up or down list,
    // calling init() to set the UUID or changing a data field
    // in one of the children nodes.
    bool changed = false;
    // Track if the node has been initted. I could just check the
    // UUID for this but setting a bool when init is called is a bit
    // easier.
    bool initted = false;
    
    Node() = default;
    // Note: Copying a node will copy its UUID, you may want to
    // rerun init() on the copy if you want it to be a different
    // entity with the same up/down lists.
    Node(const Node& copy) = default;
    virtual ~Node() = default;
    
    // Set the id field
    virtual void init() {
      std::lock_guard<std::mutex> lock(nodeMutex);
      changed = true;
      initted = true;
      boost::uuids::time_generator_v7 generator;
      id = generator();
    }

    // Find a node ID in a vector
    PtrType findIn(const std::string& id, std::vector<PtrType> &list) {
      PtrType ret;
      for (auto item : list) {
        if (id == item->idString()) {
          ret = item;
          break;
        }
      }
      return ret;
    }

    // Find a node ID in our uplist
    PtrType findUp(const std::string& id) {
      return findIn(id, up);
    }

    // Find a nide in our downlist
    PtrType findDown(const std::string& id) {
      return findIn(id, down);
    }

    // Add a node to a vector
    void addNode(PtrType node, std::vector<PtrType> &list) {
      if (!findIn(node->idString(), list)) {
        list.push_back(node);
      }
    }

    // Add a node to our uplist
    void addUp(PtrType node) {
      addNode(node, up);
    }

    // Add a node to our downlist
    void addDown(PtrType node) {
      addNode(node, down);
    }

    // Returns ID as string. Note init must be called to actually
    // set the node.
    std::string idString() const {
      std::string ret = boost::uuids::to_string(id);
      return ret;
    }

    // Set UUID from string -- Database load needs this.

    void setUuid(const std::string &uuid) {
      boost::uuids::string_generator generator;
      id = generator(uuid);
      changed = true;
    }

    // Return Node Type -- The C++ type system is very strong but a lot of it
    // is compile-time only. I want to be able to query node types from other
    // languages at run time.

    virtual std::string getNodeType() const {
      return "Node";
    }
    

    // Convert this object all its parents and children to a JSON representation
    // using the cerial serialization functions
    
    virtual std::string to_json() {
      // Should not need to lock here, because the serializers already
      // do.
      std::stringstream stream;
      {
	cereal::JSONOutputArchive archive(stream);
	archive(cereal::make_nvp(getNodeType(), shared_from_this()));
      }
      return stream.str();
    }

    template <class Archive>
    void save(Archive& ar) const {
      std::lock_guard<std::mutex> lock(nodeMutex);
      ar(cereal::make_nvp("id", boost::uuids::to_string(id)));
      ar(cereal::make_nvp("upList", up));
      ar(cereal::make_nvp("downList", down));
    }

    template <class Archive>
    void load(Archive& ar) {
      std::lock_guard<std::mutex> lock(nodeMutex);
      boost::uuids::string_generator generator;
      std::string uuid_str;
      ar(uuid_str);
      id = generator(uuid_str);
      ar(up);
      ar(down);
    }

  };
  
}
