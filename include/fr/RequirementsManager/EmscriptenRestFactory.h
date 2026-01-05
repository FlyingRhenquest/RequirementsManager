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

#include <emscripten/fetch.h>
#include <format>
#include <fr/RequirementsManager.h>
#include <fr/RequirementsManager/RestFactoryApi.h>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>
#include <memory>
#include <vector>

namespace fr::RequirementsManager {

  /**
   * Since emscripten uses old timey C functions for its
   * fetch API, I more or less *have* to set this class up
   * as a singleton to preserve its object oriented...ness...
   *
   * TODO: Hmm, looks like attr has a userdata field I missed that
   * I could pass a void pointer to this through. If the Singleton
   * causes problems, I could modify the design to do that.
   * The class methods would still need to be static though,
   * as the pointer won't be where a regular member would be
   * expecting to find it.
   *
   * This does have some implications, as anyone who signs up
   * for notifications will receive all notifications that
   * come through the factory. This should be OK for everything
   * I'm going to use it for. You can track your subscriptions
   * on the client side and disconnect them if you know when
   * you're done receiving notifications.
   */
  
  class EmscriptenServerLocatorFactory : public ServerLocatorNodeFactory {
    // These also have to be static so they don't receive a "this"
    // pointer when they're called
    static void success(emscripten_fetch_t *ctx) {
      std::string data(ctx->data, ctx->numBytes);
      // Deserialize data to a vector of shared pointers
      std::vector<std::shared_ptr<ServerLocatorNode>> nodes;
      std::stringstream dataStream(data);
      {
        cereal::JSONInputArchive archive(dataStream);
        archive(nodes);
      }
      // Signal new nodes available
      for (auto node : nodes) {        
        EmscriptenServerLocatorFactory::instance().available(node);
      }
      emscripten_fetch_close(ctx);
    }
    
    // Callback to call when emscription download encounters an error
    static void fail(emscripten_fetch_t *ctx) {
      std::string msg = std::format("Bad response from server: {}", ctx->status);
      EmscriptenServerLocatorFactory::instance().error(msg);
      emscripten_fetch_close(ctx);
    }

    EmscriptenServerLocatorFactory() {}
    
  public:

    static EmscriptenServerLocatorFactory& instance() {
      static EmscriptenServerLocatorFactory factory;
      return factory;
    }
    
    virtual ~EmscriptenServerLocatorFactory() = default;

    void fetch(const std::string& url) {
      emscripten_fetch_attr_t attr;
      emscripten_fetch_attr_init(&attr);
      strcpy(attr.requestMethod, "GET");
      attr.onsuccess = EmscriptenServerLocatorFactory::success;
      attr.onerror = EmscriptenServerLocatorFactory::fail;
      attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
      emscripten_fetch(&attr, url.c_str());
    }
    
  };

  /**
   * Likewise the graph node factory also has to be a singleton
   */

  class EmscriptenGraphNodeFactory : public GraphNodeFactory {
    static void success(emscripten_fetch_t *ctx) {
      std::cout << "Fetch success" << std::endl;
      std::string data(ctx->data, ctx->numBytes);
      std::shared_ptr<Node> node;
      std::cout << "Data is: " << data << std::endl;
      std::cout << "Deserializing data" << std::endl;
      try {
        std::cout << "Creating data stream" << std::endl;
        std::stringstream dataStream(data);
        std::cout << "Creating archive" << std::endl;
        {
          cereal::JSONInputArchive archive(dataStream);
          std::cout << "Calling archive" << std::endl;
          archive(node);
          std::cout << "Archive complete" << std::endl;
        }
      } catch (cereal::Exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
      }
      std::cout << "Signaling available" << std::endl;
      EmscriptenGraphNodeFactory::instance().available(node);
      emscripten_fetch_close(ctx);
      std::cout << "Fetch complete" << std::endl;
    }

    static void fail(emscripten_fetch_t *ctx) {
      std::string msg = std::format("Bad response from server: {}", ctx->status);
      EmscriptenGraphNodeFactory::instance().error(msg);
      emscripten_fetch_close(ctx);
    }

  public:

    static EmscriptenGraphNodeFactory& instance() {
      static EmscriptenGraphNodeFactory factory;
      return factory;
    }

    void fetch(const std::string& url) {
      emscripten_fetch_attr_t attr;
      std::cout << "Fetching graph from " << url << std::endl;
      emscripten_fetch_attr_init(&attr);
      strcpy(attr.requestMethod, "GET");
      attr.onsuccess = EmscriptenGraphNodeFactory::success;
      attr.onerror = EmscriptenGraphNodeFactory::fail;
      attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
      emscripten_fetch(&attr, url.c_str());
    }
  };
  
}
