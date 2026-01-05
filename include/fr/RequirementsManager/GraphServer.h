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

#include <atomic>
#include <condition_variable>
#include <fr/RequirementsManager/GraphNodeLocator.h>
#include <fr/RequirementsManager/PqDatabase.h>
#include <fr/RequirementsManager/PqNodeFactory.h>
#include <fr/RequirementsManager/ThreadPool.h>
#include <fr/RequirementsManager/ServerLocatorNode.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <pistache/common.h>
#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>
#include <sstream>

namespace fr::RequirementsManager {

  /**
   * This is a REST server that reads GraphNodes from the database
   * and provides the client with JSON-serialized graphs of nodes.
   * It can also handle post requests to store serialized graphs
   * back into the database.
   */

  template <typename WorkerThreadType>
  class GraphServer {
    // Set shutdown true to shut the service down.
    std::atomic<bool> _shutdown;
    // Will be true while the server is running.
    std::atomic<bool> _running;
    // Thread the server runs in
    std::thread _serverThread;
    // Threadpool to dispatch load and save requests to
    std::shared_ptr<ThreadPool<WorkerThreadType>> _threadpool;
    // Graphs endpoint name;
    std::string _graphsEndpoint;
    // Graph endpoint name
    std::string _graphEndpoint;
    // Pistache Router
    Pistache::Rest::Router _router;
    // Pistache endpoint/server
    Pistache::Http::Endpoint _server;
    // Server port
    int _port;

    void error(Pistache::Http::ResponseWriter& response, const std::string& wat, Pistache::Http::Code code = Pistache::Http::Code::Bad_Request) {
      response.send(code, wat);
    }

    // Try to retrieve the URL given the HTTP request
    std::string url(const Pistache::Http::Request& request) {
      // See if we have an X-Forwarded-Proto header. If we have
      // one, we're hiding behind ngnix and it's terminating
      // an https connection at a service endpoint.
      std::string scheme("http://");
      auto forwarded = request.headers().tryGetRaw("X-Forwarded-Proto");
      if (forwarded && forwarded->value() == "https") {
        scheme = "https://";
      }
      std::string ret(scheme);
      // Host should still be fine for constructing the rest
      // of the URL
      auto host = request.headers().tryGet<Pistache::Http::Header::Host>();
      // Default to localhost if there isn't one.
      std::string hostname = host ? host->host() : "localhost";
      ret.append(hostname);
      ret.append(":");
      std::string port = host ? host->port().toString() : std::to_string(_port);
      ret.append(port);
      
      return ret;
    }
    
    // Return all graphs
    std::vector<std::shared_ptr<ServerLocatorNode>> graphs(const Pistache::Rest::Request request) {
      std::vector<std::shared_ptr<ServerLocatorNode>> ret;
      GraphNodeLocator locator;
      locator.query();
      std::string baseUrl = url(request);
      for(auto [id, title] : locator.nodes) {
        std::string resourceUrl(baseUrl);
        resourceUrl.append("/");
        resourceUrl.append(_graphEndpoint);
        resourceUrl.append("/");
        resourceUrl.append(id);
        auto node = std::make_shared<ServerLocatorNode>(id, title, resourceUrl);
        node->init();
        ret.push_back(node);
      }
      return ret;
    }

    // Return one graph. This will block until the entire graph query
    // returns from the database. This can return an empty pointer
    // if there wasn't one in the database
    std::shared_ptr<Node> graph(const std::string id) {
      std::shared_ptr<Node> ret;
      // Threadpool needs a shared_ptr. It shouldn't be too time
      // consuming to just allocate one on the heap per-request
      // right now. If this needs to scale to a huge scale,
      // we might consider modifying factory to accept nodes
      // to load via an API call and maintaining a vector of
      // factories that we can assign to requests and then
      // placed back in the vector once they've completed.
      auto factory = std::make_shared<PqNodeFactory<WorkerThreadType>>(id);
      // Now we need to be vewwwwwy caweful about the order
      // we do things here or we're gonna have a bad time

      // Synchronization stuff so we can block until the factory
      // is truly done processing.
      std::condition_variable waitcondition;
      std::mutex waitmutex;
      bool processingComplete = false;
      factory->done.connect([&waitcondition, &processingComplete](const std::string& id) {
        processingComplete = true;
        waitcondition.notify_one();
      });
      // Now we can submit the factory to the threadpool for processing
      _threadpool->enqueue(factory);
      // Und now ve vait!
      std::unique_lock lock(waitmutex);
      // If we don't return processingComplete here, the condition could wake up
      // due to some other signal. This will solidly return only when the factory
      // processing is complete.
      waitcondition.wait(lock, [&processingComplete](){return processingComplete;});
      return(factory->getNode());
    }

    /**
     * Store a graph to the database. Graph will always be written
     * whether the UUID existed in the database previously or not.
     * This should be OK.
     *
     * Note that nodes have a changed flag, but currently this is
     * ignored and all nodes are written to the database. At
     * some point I'm probably going to want to fix that so that
     * only nodes you changed explicitly get saved, but I'll
     * need to update all the node APIs so that all the setters
     * change that flag to true if they get called and I'm going
     * to wait until I want to do a large-ish refactor before
     * I do that.
     *
     * I don't actually have to block here, so I'm not gonna.
     * Once we've created a SaveNodesNode and dispatched it
     * to the threadpool for processing, there's no need
     * to wait around.
     */

    void postGraph(std::shared_ptr<Node> graph) {
      auto saver = std::make_shared<SaveNodesNode<WorkerThreadType>>(graph);
      _threadpool->enqueue(saver);
    }
    
    /**
     * Set up routes
     *
     * This only runs once at server creation time so we can
     * set our other variables here too
     */

    void setupRoutes() {
      _running = false;
      _shutdown = false;
      
      auto graphsRoute = [&](const Pistache::Rest::Request &request,
                             Pistache::Http::ResponseWriter response) {
        std::vector<std::shared_ptr<ServerLocatorNode>> nodes = graphs(request);
        std::stringstream stream;
        {
          cereal::JSONOutputArchive archive(stream);
          archive(nodes);
        }
        response.send(Pistache::Http::Code::Ok, stream.str());
        return Pistache::Rest::Route::Result::Ok;
      };

      auto graphRoute = [&](const Pistache::Rest::Request &request,
                            Pistache::Http::ResponseWriter response) {
        auto id = request.param(":id").as<std::string>();
        if (id.empty()) {
          error(response, "Empty/No ID specified");
        } else {
          auto node = graph(id);
          if (!node) {
            error(response, "ID not found", Pistache::Http::Code::Not_Found);
          } else {                                        
            std::stringstream stream;
            {
              cereal::JSONOutputArchive archive(stream);
              archive(node);
            }
            response.send(Pistache::Http::Code::Ok, stream.str());
          }
        }
        return Pistache::Rest::Route::Result::Ok;
      };

      auto postRoute = [&](const Pistache::Rest::Request &request,
                           Pistache::Http::ResponseWriter response) {
        std::string body = request.body();
        // Note... Ok, WARNING: I'm just deserializing raw
        // request data here and that would NOT BE OK
        // in a production environment. There are NO CONTROLS
        // OF ANY KIND going on right here. If you're going
        // to use this code in any sort of real-life setting,
        // you're going to want to sanitize and validate your
        // data! Anywhoo, see the bit in the license about
        // the warranties you're not getting with this
        // project. Don't expect the front end to do it
        // for you either -- the client might not even
        // be USING one of those if it's malicious. And
        // don't trust SSL to save you, either. It's still
        // easy to implement a MITM attack through SSL.
        auto node = std::make_shared<Node>();
        std::stringstream stream(body);
        {
          cereal::JSONInputArchive archive(stream);
          archive(node);
        }
        postGraph(node);
        response.send(Pistache::Http::Code::Ok, "OK");
        return Pistache::Rest::Route::Result::Ok;
      };
      // TODO: Implement database delete, then we can
      // expose a delete endpoint here.
      Pistache::Rest::Routes::Get(_router, "/graphs", graphsRoute);
                                        
      Pistache::Rest::Routes::Get(_router, "/graph/:id", graphRoute);

      Pistache::Rest::Routes::Post(_router, "graph/:id", postRoute);
                               
    }

  public:

    GraphServer(int port) : _server({Pistache::Ipv4::any(), Pistache::Port(port)}),
                            _graphEndpoint("graph"),
                            _graphsEndpoint("graphs"),
                            _port(port)
    {
      setupRoutes();
    }

    ~GraphServer() {
      if (_running) {
        shutdown();
      }
    }

    /**
     * Start the server. The server will kick off a thread so it
     * can run in the background, will allocate endpointThreads threads
     * to Pistache for endpoint http handling and threadpoolThreads threads
     * to the threadpool for loading and saving data in the database.
     */
    
    void start(unsigned int endpointThreads, unsigned int threadPoolThreads) {
      if (!_running) {
        _threadpool = std::make_shared<ThreadPool<WorkerThreadType>>();
        _threadpool->startThreads(threadPoolThreads);
        _serverThread = std::thread([&]() {
          _running = true;
          _shutdown = false;
          auto opts = Pistache::Http::Endpoint::options().threads(endpointThreads);
          _server.init(opts);
          _server.setHandler(_router.handler());
          _server.serve();          
        });
      } else {
        throw(std::runtime_error("Server is already running"));
      }
    }

    void shutdown() {
      if (_running) {
        _shutdown = true;
        _threadpool->shutdown();
        _server.shutdown();
        _serverThread.join();
        _threadpool->join();
        _running = false;
      }
    }
    
  };
  
}
