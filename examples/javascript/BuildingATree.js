// Assemble a small tree and output it as json

// To try this, copy thios to the directory where you
// built FRRequirementsManagerModule.js and run
// node BuildingATree.js. If you get json back, hooray!

const ManagerFactory = require("./FRRequirementsManagerModule.js")

ManagerFactory().then((Factory) => {
    var org = new Factory.Organization();
    var product = new Factory.Product();

    // Connectnodes connects two nodes via their up/down
    // lists, with the first node being the parent and the
    // second node being the child. connectNodes also
    // calls init on both objects, which assigns their
    // unique identifiers. Only objects that haven't already
    // been initted will be initted so you can safely use it
    // to build bigger trees without clobbering UUIDs.

    Factory.connectNodes(org, product);

    // A fictional company
    org.setName("Global Consolidated Software Engineering, Inc.");
    product.setTitle("Requirements Manager");
    product.setDescription("Requirements manager that I'd like to use");

    var project = new Factory.Project();
    project.setName("Requirements Manager MVP");
    project.setDescription("Minimum viable project design and implementation");
    
    Factory.connectNodes(product, project);

    var rtn = new Factory.Text();
    var rtn2 = new Factory.Text();
    rtn.setText("Random text node");
    rtn2.setText("Another random text node");
    
    Factory.connectNodes(project, rtn);
    Factory.connectNodes(project, rtn2);

    console.log(org.to_json());

    // Really should delete all those things but they're
    // smart pointers and will get cleaned up whenever
    // javascript GCs anyway.
    
});
