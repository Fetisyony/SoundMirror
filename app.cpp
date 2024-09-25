#include "app.h"


int main() {
    int rc = OK;

    Merger *merger = new Merger();
    rc = merger->start();
    delete merger;

    return rc;
}
