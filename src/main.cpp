// Simpler version of example at https://gitlab.com/painlessMesh/painlessMesh/-/blob/develop/examples/startHere/startHere.ino


#include <painlessMesh.h>

#define   MESH_SSID       "test1"
#define   MESH_PASSWORD   "meshlongpassword"
#define   MESH_PORT       5555


Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;
SimpleList<uint32_t> nodes;

void sendMessage();
Task taskSendMessage( TASK_SECOND * 1, TASK_FOREVER, &sendMessage ); // start with a one second interval
bool calc_delay = false;

void cb_NewConnection(uint32_t nodeId);
void cb_ChangedConnections(); 
void cb_Receive(uint32_t from, String & msg);
void cb_NodeTimeAdjusted(int32_t offset); 
void cb_NodeDelayReceived(uint32_t from, int32_t delay);

void setup()
{
  Serial.begin(115200);
  mesh.setDebugMsgTypes(ERROR | DEBUG);  // set before init() so that you can see error messages

  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);

  mesh.onNewConnection(&cb_NewConnection);
  mesh.onChangedConnections(&cb_ChangedConnections);
  mesh.onReceive(&cb_Receive);
  mesh.onNodeTimeAdjusted(&cb_NodeTimeAdjusted);
  mesh.onNodeDelayReceived(&cb_NodeDelayReceived);

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();
  randomSeed(analogRead(A0));

  Serial.printf("[setup] Init of node %lu DONE!!\r\n", mesh.getNodeId());
}

void loop()
{
  mesh.update();
}

void cb_NewConnection(uint32_t nodeId) {
  Serial.printf("[NewConnection] nodeId = %u\r\n", nodeId);
  Serial.printf("[NewConnection] %s\r\n", mesh.subConnectionJson(true).c_str());
}

void cb_ChangedConnections() {
  nodes = mesh.getNodeList();

  Serial.printf("[ChangedConnections] Num nodes: %d\r\n", nodes.size());
  Serial.print("[ChangedConnections] Connection list:\r\n");

  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end()) {
    Serial.printf("[ChangedConnections] %u\r\n", *node);
    node++;
  }
  calc_delay = true;
}

void cb_Receive(uint32_t from, String & msg) {
  Serial.printf("[Receive] from %u msg=%s\r\n", from, msg.c_str());
}

void cb_NodeTimeAdjusted(int32_t offset) {
  Serial.printf("[NodeTimeAdjusted] Adjusted time %u. Offset = %d\r\n", mesh.getNodeTime(), offset);
}

void cb_NodeDelayReceived(uint32_t from, int32_t delay) {
  Serial.printf("[NodeDelayReceived] Delay to node %u is %d us\r\n", from, delay);
}

void sendMessage() {
  String msg = "Hello from node ";
  msg += mesh.getNodeId();
  msg += " myFreeMemory: " + String(ESP.getFreeHeap());
  mesh.sendBroadcast(msg);

  if (calc_delay) {
    SimpleList<uint32_t>::iterator node = nodes.begin();
    while (node != nodes.end()) {
      // Response in cb_NodeDelayReceived
      mesh.startDelayMeas(*node);
      node++;
    }
    calc_delay = false;
  }

  Serial.printf("[sendMessage] %s\r\n", msg.c_str());
  taskSendMessage.setInterval( random(TASK_SECOND * 5, TASK_SECOND * 30));  // between 5 and 30 seconds
}

