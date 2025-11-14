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
#include <sstream>
#include <string>


namespace fr::RequirementsManager {

  /**
   * A node in a Requirements graph. Many entities can be nodes,
   * this specifies the basic API of all those entities
   */

  struct Node {
    using PtrType = std::shared_ptr<Node>;

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
    Node(Node&& toMove) = default;
    virtual ~Node() = default;
    
    virtual Node& operator=(Node&& toMove) = default;

    // Set the id field
    virtual void init() {
      changed = true;
      initted = true;
      boost::uuids::time_generator_v7 generator;
      id = generator();
    }

    // Returns ID as string. Note init must be called to actually
    // set the node.
    std::string idString() {
      std::string ret = boost::uuids::to_string(id);
      return ret;
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
      std::stringstream stream;
      {
	cereal::JSONOutputArchive archive(stream);
	archive(cereal::make_nvp(getNodeType(), *this));
      }
      return stream.str();
    }

    template <class Archive>
    void save(Archive& ar) const {
      ar(cereal::make_nvp("id", boost::uuids::to_string(id)));
      ar(cereal::make_nvp("upList", up));
      ar(cereal::make_nvp("downList", down));
    }

    template <class Archive>
    void load(Archive& ar) {
      boost::uuids::string_generator generator;
      std::string uuid_str;
      ar(uuid_str);
      id = generator(uuid_str);
      ar(up);
      ar(down);      
    }

  };
  
}
