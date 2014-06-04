#include "server.h"

int main()
{
    cSceneContainer scene;
    cServer         myServer(999, 2, 54000, &scene);    // protocol ID, expected clients, port #, scenecontainer*
    myServer.run();
    return 0;
}