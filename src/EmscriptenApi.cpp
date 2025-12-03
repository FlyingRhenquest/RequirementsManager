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

#include <emscripten/bind.h>
#include <fr/RequirementsManager.h>

using namespace emscripten;
using namespace fr::RequirementsManager;

EMSCRIPTEN_BINDINGS(FRRequirementsManager) {
  register_vector<std::shared_ptr<Node>>("VectorNode");
  
  function("connectNodes", &connectNodes);

  /**
   * Node binding for Javascript.
   */
  
  class_<Node>("Node")
    .smart_ptr_constructor("Node", &std::make_shared<Node>)
    .property("up", &Node::up, return_value_policy::reference())
    .property("down", &Node::down, return_value_policy::reference())
    .property("changed", &Node::changed, return_value_policy::reference())
    .function("idString", &Node::idString)
    .function("setUuid", &Node::setUuid)
    .function("init", &Node::init)
    .function("to_json", &Node::to_json);

  class_<CommitableNode, base<Node>>("CommitableNode")
    .smart_ptr_constructor("CommitableNode", &std::make_shared<CommitableNode>)
    .function("commit", &CommitableNode::commit)
    .function("isCommitted", &CommitableNode::isCommitted)
    .function("getChangeChild", &CommitableNode::getChangeChild)
    .function("getChangeParent", &CommitableNode::getChangeParent)
    .function("addChangeChild", &CommitableNode::addChangeChild)
    .function("discardChange", &CommitableNode::discardChange);

  class_<Organization, base<Node>>("Organization")
    .smart_ptr_constructor("Organization", &std::make_shared<Organization>)
    .function("isLocked", &Organization::isLocked)
    .function("setName", &Organization::setName)
    .function("getName", &Organization::getName)
    .function("lock", &Organization::lock)
    .function("unlock", &Organization::unlock);
  
  class_<Product, base<CommitableNode>>("Product")
    .smart_ptr_constructor("Product", &std::make_shared<Product>)
    .function("setTitle", &Product::setTitle)
    .function("setDescription", &Product::setDescription)
    .function("getTitle", &Product::getTitle)
    .function("getDescription", &Product::getDescription);

  class_<Project, base<Node>>("Project")
    .smart_ptr_constructor("Project", &std::make_shared<Project>)
    .function("setName", &Project::setName)
    .function("setDescription", &Project::setDescription)
    .function("getName", &Project::getName)
    .function("getDescription", &Project::getDescription);

  class_<Requirement, base<CommitableNode>>("Requirement")
    .smart_ptr_constructor("Requirement", &std::make_shared<Requirement>)
    .function("setTitle", &Requirement::setTitle)
    .function("setText", &Requirement::setText)
    .function("setFunctional", &Requirement::setFunctional)
    .function("getTitle", &Requirement::getTitle)
    .function("getText", &Requirement::getText)
    .function("isFunctional", &Requirement::isFunctional);

  class_<Story, base<CommitableNode>>("Story")
    .smart_ptr_constructor("Story", &std::make_shared<Story>)
    .function("getTitle", &Story::getTitle)
    .function("setTitle", &Story::setTitle)
    .function("getGoal", &Story::getGoal)
    .function("setGoal", &Story::setGoal)
    .function("getBenefit", &Story::getBenefit)
    .function("setBenefit", &Story::setBenefit);

  class_<UseCase, base<CommitableNode>>("UseCase")
    .smart_ptr_constructor("UseCase", &std::make_shared<UseCase>)
    .function("setName", &UseCase::setName)
    .function("getName", &UseCase::getName);

  class_<Text, base<Node>>("Text")
    .smart_ptr_constructor("Text", &std::make_shared<Text>)
    .function("setText", &Text::setText)
    .function("getText", &Text::getText);

  class_<Completed, base<Node>>("Completed")
    .smart_ptr_constructor("Completed", std::make_shared<Completed>)
    .function("setDescription", &Completed::setDescription)
    .function("getDescription", &Completed::getDescription);

  class_<KeyValue, base<Node>>("KeyValue")
    .smart_ptr_constructor("KeyValue", &std::make_shared<KeyValue>)
    .function("setKey", &KeyValue::setKey)
    .function("getKey", &KeyValue::getKey)
    .function("setValue", &KeyValue::setValue)
    .function("getValue", &KeyValue::getValue);

  class_<TimeEstimate, base<Node>>("TimeEstimate")
    .smart_ptr_constructor("TimeEstimate", &std::make_shared<TimeEstimate>)
    .function("setText", &TimeEstimate::setText)
    .function("getText", &TimeEstimate::getText)
    .function("setEstimate", &TimeEstimate::setEstimate)
    .function("getEstimate", &TimeEstimate::getEstimate);

  class_<Effort, base<Node>>("Effort")
    .smart_ptr_constructor("Effort", &std::make_shared<Effort>)
    .function("setText", &Effort::setText)
    .function("getText", &Effort::getText)
    .function("setEffort", &Effort::setEffort)
    .function("getEffort", &Effort::getEffort);

  class_<Role, base<Node>>("Role")
    .smart_ptr_constructor("Role", &std::make_shared<Role>)
    .function("setWho", &Role::setWho)
    .function("getWHo", &Role::getWho);

  class_<Actor, base<Node>>("Actor")
    .smart_ptr_constructor("Actor", &std::make_shared<Actor>)
    .function("getActor", &Actor::getActor)
    .function("setActor", &Actor::setActor);

  class_<Goal, base<Node>>("Goal")
    .smart_ptr_constructor("Goal", &std::make_shared<Goal>)
    .function("setAction", &Goal::setAction)
    .function("setOutcome", &Goal::setOutcome)
    .function("setContext", &Goal::setContext)
    .function("setTargetDate", &Goal::setTargetDate)
    .function("setTargetDateConfidence", &Goal::setTargetDateConfidence)
    .function("setAlignment", &Goal::setAlignment)
    .function("getAction", &Goal::getAction)
    .function("getOutcome", &Goal::getOutcome)
    .function("getContext", &Goal::getContext)
    .function("getTargetDate", &Goal::getTargetDate)
    .function("getTargetDateConfidence", &Goal::getTargetDateConfidence);

  class_<Purpose, base<Node>>("Purpose")
    .smart_ptr_constructor("Purpose", &std::make_shared<Purpose>)
    .function("setDescription", &Purpose::setDescription)
    .function("setDeadline", &Purpose::setDeadline)
    .function("setDeadlineConfidence", &Purpose::setDeadlineConfidence)
    .function("getDescription", &Purpose::getDescription)
    .function("getDeadline", &Purpose::getDeadline)
    .function("getDeadlineConfidence", &Purpose::getDeadlineConfidence);

  class_<Person, base<Node>>("Person")
    .smart_ptr_constructor("Person", &std::make_shared<Person>)
    .function("setFirstName", &Person::setFirstName)
    .function("setLastName", &Person::setLastName)
    .function("getFirstName", &Person::getFirstName)
    .function("getLastName", &Person::getLastName);

  class_<InternationalAddress, base<Node>>("InternationalAddress")
    .smart_ptr_constructor("InternationalAddress", &std::make_shared<InternationalAddress>)
    .function("setCountryCode", &InternationalAddress::setCountryCode)
    .function("setAddressLines", &InternationalAddress::setAddressLines)
    .function("setLocality", &InternationalAddress::setLocality)
    .function("setPostalCode", &InternationalAddress::setPostalCode)
    .function("getCountryCode", &InternationalAddress::getCountryCode)
    .function("getLocality", &InternationalAddress::getLocality)
    .function("getPostalCode", &InternationalAddress::getPostalCode);

  class_<Event, base<Node>>("Event")
    .smart_ptr_constructor("Event", &std::make_shared<Event>)
    .function("getName", &Event::getName)
    .function("setName", &Event::setName)
    .function("getDescription", &Event::getDescription)
    .function("setDescription", &Event::setDescription);			   
    
}
