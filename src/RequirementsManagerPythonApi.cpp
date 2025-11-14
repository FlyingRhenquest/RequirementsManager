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

#include <fr/RequirementsManager.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/bind_vector.h>
#include <nanobind/stl/chrono.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <memory>

using namespace fr::RequirementsManager;

/**
 * OK, this is going to require a moment. If you're sitting
 * there wondering why I did *this* specifically, it seems like
 * simply forcing nanobind to return shared pointers in all my
 * constructors and exposing my up/down list vectors in node
 * is *NOT ENOUGH* to be able to place nodes and their children
 * in the up/down lists in python using append. Append just sort
 * of quietly fails if you try to append a THING of WRONG TYPE
 * in it and you're left scatchign your head wondering why
 * your vector constantly has nothing in it.
 *
 * You have to first make the vector of shared pointers of nodes
 * opaque so nanobind doesn't just default to whatever it's trying to
 * do which apparently is wrong.
 *
 * Once you make it opaque on the line below this comment block,
 * you have to use nanobind::bind_vector to bind the shared
 * vector of shared points to some type (doesn't seem to matter
 * what, I used "NodeVector"). Then you can just .def_rw your
 * vectors normally in your class API definitions, and that
 * actually seems to work. Also, nanobind doesn't seem to
 * include similar bind functionality for std::list. I checked.
 */
NB_MAKE_OPAQUE(std::vector<std::shared_ptr<Node>>);

NB_MODULE(FRRequirements, m) {

  // Python API for RequirementsManager objects

  // Tell Nanobind vectors of nodes use shared pointers
  // nanobind::bind_vector<std::vector<std::shared_ptr<Node>>>(m, "NodeVector");
  
  // GetChangeNode overload for various CommitableTypes

  // m.def("getChangeNode", &getChangeNode<CommitableNode>);
  m.def("getChangeNode", &getChangeNode<Requirement>);
  m.def("getChangeNode", &getChangeNode<Story>);
  m.def("getChangeNode", &getChangeNode<UseCase>);

  m.def("connectNodes", &connectNodes);

  nanobind::bind_vector<std::vector<std::shared_ptr<Node>>>(m, "NodeVector");

  // Node -- You won't tend to use this one directly --
  // other htings inherit from it

  nanobind::class_<Node>(m, "Node")
    .def(nanobind::new_([](){return std::make_shared<Node>();}))
    .def_rw("up", &Node::up, "Node up-list. This indicates some sort of owner/parent relationship.")
    .def_rw("down", &Node::down, "Node down-list. This indicates some sort of owned/child relationship")
    .def_rw("changed", &Node::changed, "Indicates some data in the node changed.")
    .def("idString", &Node::idString, "Unique (UUIDV7) ID. You must call init to set the id, initally.")
    .def("init", &Node::init, "Sets/Resets the Node's ID.")
    .def("to_json", &Node::to_json, "Returns JSON for this node. Includes entire graph accessible by node up and down lists.");
  
  // CommitableNode -- You won't tend to use this one
  // directly -- other things inherit from it
  
  nanobind::class_<CommitableNode,Node>(m, "CommitableNode")
    .def(nanobind::new_([](){return std::make_shared<CommitableNode>();}))
    .def("commit", &CommitableNode::commit, "Commits this node, making further change impossible. This is for traceability -- to make a change to a committed node, create a pointer of this node type and call addChangeChild to the node you want to add the change to.")
    .def("isCommitted", &CommitableNode::isCommitted, "Checks committed status of this node.")
    .def("getChangeChild", &CommitableNode::getChangeChild, "Traverses the change child nodes. You can call getChangeChild recursively on change children until you receieve a None back. None indicates there are no more change children.")
    .def("getChangeParent", &CommitableNode::getChangeParent, "Traverses the change parent nodes. It will return None if you're at the top level parent.")
    .def("addChangeChild", &CommitableNode::addChangeChild, "Add a change child at the end of the changeChild tree.")
    .def("discardChange", &CommitableNode::discardChange, "Attempt to discard the changeChild of the current node. The object will refuse to do so if the current changeChild is already committed.");

  nanobind::class_<Organization,Node>(m, "Organization")
    .def(nanobind::new_([](){return std::make_shared<Organization>();}))
    .def("isLocked", &Organization::isLocked, "Checks to see if the organization is locked. If it's locked you can't change its name.")
    .def("setName", &Organization::setName, "Sets the name of the organization")
    .def("getName", &Organization::getName, "Gets the name of the organization")
    .def("lock", &Organization::lock, "Locks the organization, preventing its name from being changed.")
    .def("unlock", &Organization::unlock, "Unlocks the organization, allowing its name to be changed.");

  nanobind::class_<Product, CommitableNode>(m, "Product")
    .def(nanobind::new_([](){return std::make_shared<Product>();}))
    .def("setTitle", &Product::setTitle, "Set the product title")
    .def("setDescription", &Product::setDescription, "Set the product description")
    .def("getTitle", &Product::getTitle, "Get the product name")
    .def("getDescription", &Product::getDescription, "Get the product description");
  
  nanobind::class_<Project, Node>(m, "Project")
    .def(nanobind::new_([](){return std::make_shared<Project>();}))
    .def("setName", &Project::setName, "Sets the project name")
    .def("setDescription", &Project::setDescription, "Sets the project description")
    .def("getName", &Project::getName, "Gets the project name")
    .def("getDescription", &Project::getDescription, "Gets the project description");

  nanobind::class_<Requirement,CommitableNode>(m, "Requirement")
    .def(nanobind::new_([](){return std::make_shared<Requirement>();}))
    .def("setTitle", &Requirement::setTitle, "Set the requirement title")
    .def("setText", &Requirement::setText, "Set the requirement text")
    .def("setFunctional", &Requirement::setFunctional, "Set whether the requirement is functional or non-functional. Pass it a True/False")
    .def("getTitle", &Requirement::getTitle, "Get the requirement title")
    .def("getText", &Requirement::getText, "Get the requirement text")
    .def("isFunctional", &Requirement::isFunctional, "Returns true if the requirement is a functional requirement")
    .def("to_json", &Requirement::to_json, "Returns JSON representation of this requirement, including the entire graph of up/down nodes");

  nanobind::class_<Story, CommitableNode>(m, "Story")
    .def(nanobind::new_([](){return std::make_shared<Story>();}))
    .def("getTitle", &Story::getTitle, "Get the Story title")
    .def("setTitle", &Story::setTitle, "Set the Story title")
    .def("getGoal", &Story::getGoal, "Get the story goal")
    .def("setGoal", &Story::setGoal, "Set the story goal")
    .def("getBenefit", &Story::getBenefit, "Get the story benefit")
    .def("setBenefit", &Story::setBenefit, "Set the story benefit");

  nanobind::class_<UseCase, CommitableNode>(m, "UseCase")
    .def(nanobind::new_([](){return std::make_shared<UseCase>();}))
    .def("setName", &UseCase::setName, "Set the use case name")
    .def("getName", &UseCase::getName, "Get the use case name");

  nanobind::class_<Text, Node>(m, "Text")
    .def(nanobind::new_([](){return std::make_shared<Text>();}))
    .def("setText", &Text::setText, "Set the text for this node")
    .def("getText", &Text::getText, "Get the text for this node");

  nanobind::class_<Completed, Node>(m, "Completed")
    .def(nanobind::new_([](){return std::make_shared<Completed>();}))
    .def("setDescription", &Completed::setDescription, "Set the description for this completed node.")
    .def("getDescription", &Completed::getDescription, "Get the description for this completed node.");
  
  nanobind::class_<KeyValue, Node>(m, "KeyValue")
    .def(nanobind::new_([](){return std::make_shared<KeyValue>();}))
    .def("setKey", &KeyValue::setKey, "Sets the key name for this node")
    .def("getKey", &KeyValue::getKey, "Gets the key name for this node")
    .def("setValue", &KeyValue::setValue, "Sets the value of this node")
    .def("getValue", &KeyValue::getValue, "Gets the value for this node");

  nanobind::class_<TimeEstimate, Node>(m, "TimeEstimate")
    .def(nanobind::new_([](){return std::make_shared<TimeEstimate>();}))
    .def("setText", &TimeEstimate::setText, "Sets the text for this node")
    .def("getText", &TimeEstimate::getText, "Gets the text for this node")
    .def("setEstimate", &TimeEstimate::setEstimate, "Set the estimate (Duration, seconds) for this node")
    .def("getEstimate", &TimeEstimate::getEstimate, "Gets the estimate (Duration, seconds) for this node");

  nanobind::class_<Effort, Node>(m, "Effort")
    .def(nanobind::new_([](){return std::make_shared<Effort>();}))
    .def("setText", &Effort::setText, "Set the text for this node")
    .def("getText", &Effort::getText, "Get the text for this node")
    .def("setEffort", &Effort::setEffort, "Set the effort (time spent) (Duration, seconds) for this node. You can drop more than one effort node in the down list of a node where time-worked on the thing makes sense.")
    .def("getEffort", &Effort::getEffort, "Get the effort (duration, seconds) for this node");

  nanobind::class_<Role, Node>(m, "Role")
    .def(nanobind::new_([](){return std::make_shared<Role>();}))
    .def("setWho", &Role::setWho, "Sets the who value for this node")
    .def("getWho", &Role::getWho, "Gets the who value for this node");

  nanobind::class_<Actor, Node>(m, "Actor")
    .def(nanobind::new_([](){return std::make_shared<Actor>();}))
    .def("getActor", &Actor::getActor, "Gets the actor text for this node")
    .def("setActor", &Actor::setActor, "Sets the actor text for this node");

  nanobind::class_<Goal, Node>(m, "Goal")
    .def(nanobind::new_([](){return std::make_shared<Goal>();}))
    .def("setAction", &Goal::setAction, "Set the action (what will be done) part of this goal")
    .def("setOutcome", &Goal::setOutcome, "Set the outcome (What defines success) part of this goal")
    .def("setContext", &Goal::setContext, "Set the context (who/what is this goal targeting) part of this goal")
    .def("setTargetDate", &Goal::setTargetDate, "Set the target date/Deadline of the goal (Python Datetime object)")
    .def("setTargetDateConfidence", &Goal::setTargetDateConfidence, "Set target date confidence/priority/flexibiltiy (string tag)")
    .def("setAlignment", &Goal::setAlignment, "Set alignment (Why we have goal first place.)")
    .def("getAction", &Goal::getAction, "Get the action of this goal")
    .def("getOutcome", &Goal::getOutcome, "Get the outcome of this goal")
    .def("getContext", &Goal::getContext, "Get the context of this goal")
    .def("getTargetDate", &Goal::getTargetDate, "Get the target date/deadline of this goal")
    .def("getTargetDateConfidence", &Goal::getTargetDateConfidence, "Get the target date confidence/flexibility/priority (string tag)");

  nanobind::class_<Purpose, Node>(m, "Purpose")
    .def(nanobind::new_([](){return std::make_shared<Purpose>();}))
    .def("setDescription", &Purpose::setDescription, "Set the description of this purpose (IE: You pass butter)")
    .def("setDeadline", &Purpose::setDeadline, "Set the deadline for this purpose (Python datetime object)")
    .def("setDeadlineConfidence", &Purpose::setDeadlineConfidence, "Set the confidence/wiggle room/priority of this deadline (string tag)")
    .def("getDescription", &Purpose::getDescription, "Get the description of this Purpose")
    .def("getDeadline", &Purpose::getDeadline, "Get the deadline for this Purpose (Python datetime object)")
    .def("getDeadlineConfidence", &Purpose::getDeadlineConfidence, "Get the confdience/wiggle room/priority of this Purpose (String tag)");

  nanobind::class_<Person, Node>(m, "Person")
    .def(nanobind::new_([](){return std::make_shared<Person>();}))
    .def("setLastName", &Person::setLastName, "Set last name")
    .def("setFirstName", &Person::setFirstName, "Set first name")
    .def("getLastName", &Person::getLastName, "Get last name")
    .def("getFirstName", &Person::getFirstName, "Get first name");

  nanobind::class_<PhoneNumber, Node>(m, "PhoneNumber")
    .def(nanobind::new_([](){return std::make_shared<PhoneNumber>();}))
    .def("setCountryCode", &PhoneNumber::setCountryCode, "Set country code (nominally optional if you're a one country project.)")
    .def("setNumber", &PhoneNumber::setNumber, "Set phone number")
    .def("setPhoneType", &PhoneNumber::setPhoneType, "Set phone type (home, office etc)")
    .def("getCountryCode", &PhoneNumber::getCountryCode, "Get country code.")
    .def("getNumber", &PhoneNumber::getNumber, "Get phone number")
    .def("getPhoneType", &PhoneNumber::getPhoneType, "Get phone type (home, office etc)");

  nanobind::class_<InternationalAddress, Node>(m, "InternationalAddress")
    .def(nanobind::new_([](){return std::make_shared<InternationalAddress>();}))
    .def("setCountryCode", &InternationalAddress::setCountryCode, "Set country code (ISO 3166-1 Country codes)")
    .def("setAddressLines", &InternationalAddress::setAddressLines, "Set an address line Text node. If you need additional address lines, put them in the down list of the text node you set this to.")
    .def("setLocality", &InternationalAddress::setLocality, "Set Locality (city/town)")
    .def("setPostalCode", &InternationalAddress::setPostalCode, "set Postal/Zip Code")
    .def("getCountryCode", &InternationalAddress::getCountryCode, "Get country code")
    .def("getAddressLines", &InternationalAddress::getAddressLines, "Get address lines text node.")
    .def("getLocality", &InternationalAddress::getLocality, "Get locality (City/Town)")
    .def("getPostalCode", &InternationalAddress::getPostalCode, "Get Postal/Zip code");
  
  nanobind::class_<Event, Node>(m, "Event")
    .def(nanobind::new_([](){return std::make_shared<Event>();}))
    .def("getName", &Event::getName, "Gets the name of this event")
    .def("setName", &Event::setName, "Sets the name of this event")
    .def("getDescription", &Event::getDescription, "Gets the description of this event")
    .def("setDescription", &Event::setDescription, "Sets the description of this event");
	 
}
