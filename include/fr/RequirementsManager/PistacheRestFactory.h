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

#include <exception>
#include <stdexcept>
#include <format>
#include <fr/RequirementsManager.h>
#include <fr/RequirementsManager/RestFactoryApi.h>
#include <pistache/http.h>
#include <pistache/client.h>
#include <pistache/async.h>
#include <sstream>

namespace fr::RequirementsManager {

  class PistacheLocatorNodeFactory : public ServerLocatorNodeFactory {
    Pistache::Http::Experimental::Client client;

    void success(Pistache::Http::Response &response) {
      std::stringstream data(response.body());
      std::vector<std::shared_ptr<ServerLocatorNode>> nodes;
      try {
        cereal::JSONInputArchive archive(data);
        archive(nodes);
      } catch (cereal::Exception& e) {
        std::string err = std::format("Deserialization error: {}", e.what());
        this->error(err);
      }

      for (auto node : nodes) {
        this->available(node);
      }      
    }

    void fail(std::exception_ptr eptr) {
      try {
        if (eptr) {
          std::rethrow_exception(eptr);
        }
      } catch (const std::exception& e) {
        std::string err = std::format("Client error: {}", e.what());
        this->error(err);
      }
    }
    
  public:
    PistacheLocatorNodeFactory() {
      auto opts = Pistache::Http::Experimental::Client::options().threads(1);
      client.init(opts);
    }

    virtual ~PistacheLocatorNodeFactory() {
      client.shutdown();
    }
    
    void fetch(const std::string& url) {
      auto promise = client.get(url).send();
      
      promise.then(
          [&](Pistache::Http::Response response) {
            this->success(response);
          },
          [&](std::exception_ptr eptr) {
            fail(eptr);
          });        
    }
  };
  
  class PistacheGraphNodeFactory : public GraphNodeFactory {
    Pistache::Http::Experimental::Client client;
    
    void success(Pistache::Http::Response &response) {
      std::stringstream data(response.body());
      std::shared_ptr<Node> node;
      try {
        cereal::JSONInputArchive archive(data);
        archive(node);        
      } catch (cereal::Exception& e) {
        std::string err = std::format("Deserialization error: {}", e.what());
        this->error(err);
      }
      if (node) {
        this->available(node);
      }      
    }
    
    void fail(std::exception_ptr eptr) {
      try {
        if (eptr) {
          std::rethrow_exception(eptr);
        }
      } catch (std::exception &e) {
        std::string err = std::format("Client error: {}", e.what());
        this->error(err);
      }
    }

  public:
    PistacheGraphNodeFactory() {
      auto opts = Pistache::Http::Experimental::Client::options().threads(1);
      client.init(opts);
    }

    virtual ~PistacheGraphNodeFactory() {
      client.shutdown();
    }

    void fetch(const std::string &url) {
      auto promise = client.get(url).send();

      promise.then(
          [&](Pistache::Http::Response response) {
            this->success(response);
          },
          [&](std::exception_ptr eptr) {
            this->fail(eptr);
          });
    }
  };
  
}
