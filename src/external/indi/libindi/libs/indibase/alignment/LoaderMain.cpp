#include <iostream>

#include "LoaderClient.h"

using namespace std;

static LoaderClient Client;

int main(int argc, char *argv[])
{
    Client.Initialise(argc, argv);

    Client.Load();

    cout << "Press any key followed by return to terminate the client.\n";
    string term;
    cin >> term;
    return 0;
}
