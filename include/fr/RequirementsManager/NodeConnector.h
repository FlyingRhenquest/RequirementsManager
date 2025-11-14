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
 *
 */

#pragma once

#include <fr/RequirementsManager/Node.h>

namespace fr::RequirementsManager {

  // Takes shared pointers to parent and child nodes, inits them
  // if they haven't been initted yet and sets the up/down nodes
  // in both nodes to point at the other node.

  void connectNodes(Node::PtrType parent, Node::PtrType child);
  
}
