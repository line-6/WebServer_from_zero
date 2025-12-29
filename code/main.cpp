#include <unistd.h>
#include "server/webserver.h"

int main() {
    WebServer server(8080, 3, 600000, false,         
        3306, "root", "326326", "WebServer",
        12, 6, true, 0, 1024);
    server.start();
    return 0;
}