#include <iostream>
#include "client.h"
#include "sceneContainer.h"

int main()
{
    cSceneContainer myScene;
    cClient         myClient("Ketto", 999, myScene);
    
    myClient.setup("2.2.3.4", 3, 60, 10);
    if ( myClient.connect("127.0.0.1", 54000) )
    {
        std::cout << "Connected, starting game loop.\n";
        myClient.run();
    }
    else
    {
        std::cout << "Timeout.\n";
    }
    return 0;
}