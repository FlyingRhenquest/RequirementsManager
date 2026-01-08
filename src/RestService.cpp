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

#include <boost/program_options.hpp>
#include <fr/RequirementsManager/GraphServer.h>
#include <iostream>

void printHelp(const std::string& programName, const boost::program_options::options_description &desc) {

  std::cout << "Usage: " << programName << " [-p port] [-a address] [-t threads]" << std::endl;
  std::cout << "Port is optional and defaults to 8080" << std::endl;
  std::cout << "Address is optional and defaults to 127.0.0.1" << std::endl;
  std::cout << "Use address 0.0.0.0 to make the server listen on all interfaces." << std::endl;
  std::cout << std::endl << desc << std::endl;  
}

int main(int argc, char *argv[]) {
  using namespace fr::RequirementsManager;
  const std::string programName(argv[0]);
  int port = 8080;
  std::string address("127.0.0.1");
  boost::program_options::options_description desc("Options");
  desc.add_options()
    ("help,h", "Print help message")
    ("port,p",
     boost::program_options::value<int>(&port)->default_value(8080),
     "Port to listen on.")
    ("address,a",
     boost::program_options::value<std::string>(&address)->default_value("127.0.0.1"),
     "Listen address (use 0.0.0.0 to listen on all interfaces.)"
     )
    ;
     
  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
  boost::program_options::notify(vm);

  if (vm.count("help")) {
    printHelp(programName, desc);
    exit(-1);
  }

  GraphServer<WorkerThread> server(address, port);
  // TODO: Allow the user to specify thread counts
  server.start(2,2);
  std::cout << "Server started on " << address << ":" << port << std::endl;
  // TODO: Install a signal handler to handle sigint(ctrl-c)/sighup?
  server.join();
    
}
