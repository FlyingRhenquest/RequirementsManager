# Demonstrates loading and saving node graphs in Python

import FRRequirements
import time

# Create some nodes and connect them together
# Nodes don't currently have rules about how they connect
# together.

org = FRRequirements.Organization()
org.setName("Global Consolidated Software Engineering, Inc.")

proj = FRRequirements.Project()
proj.setName("Engineer some software")
FRRequirements.connectNodes(org, proj)

prod = FRRequirements.Product()
prod.setTitle("Some Software")

FRRequirements.connectNodes(proj, prod)

req = FRRequirements.Requirement()
req.setTitle("Software must be engineered")
FRRequirements.connectNodes(prod, req)
req = FRRequirements.Requirement()
req.setTitle("Software must be tested")
FRRequirements.connectNodes(prod, req)

# Yeah that should be enough

# You need a threadpool to load/save

threadpool = FRRequirements.ThreadPool()
threadpool.startThreads(4)

saver = FRRequirements.SaveNodesNode(org)
threadpool.enqueue(saver)

# You probably don't really need 3 seconds for this save
# You can check saver.treeSaveComplete() to see if the
# entire graph has been saved or you can go off and do
# other things and check later.
time.sleep(3)

loader = FRRequirements.PqNodeFactory(org.idString())
threadpool.enqueue(loader)

# You probably also don't need 3 seconds to load it.
# loader.graphLoaded() will give you some idea if your
# graph has been loaded, but it might not be entirely
# populated yet. You can also threadpool.workerStatus()
# to see if any of the threads are still working.

time.sleep(3)
loadedOrg = loader.getNode()

print("org uuid:       ", org.idString())
print("loadedOrg uuid: ", loadedOrg.idString())

# You can also check their to_json() values, but those
# are a bit unwieldy.
