#include <iostream>

#include "MathPluginManagerClient.h"

using namespace std;

static MathPluginManagerClient Client;

int main(int argc, char *argv[])
{
    Client.Initialise(argc, argv);

    Client.Test();

    cout << "Press any key followed by return to terminate the client.\n";
    string term;
    cin >> term;
    return 0;
}
