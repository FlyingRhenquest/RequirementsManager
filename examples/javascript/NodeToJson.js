// Create a Node object, init it and log its
// to_json string. This is a basic hello world level
// just to verify that you're interacting with the C++
// API.

// To try this, copy this to the directory where
// you built FRRequiremenetsManagerModule.js and run
// node NodeToJson.js. If you get json back, hooray!

const ManagerFactory = require("./FRRequirementsManagerModule.js");

ManagerFactory().then((Factory) => {
    var n = new Factory.Node();

    n.init();
    console.log(n.to_json());

    n.delete();
});
