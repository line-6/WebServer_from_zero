#include "log.h"

Log::Log() {

}

Log* Log::Instance() {
    static Log instance;
    return &instance;
}