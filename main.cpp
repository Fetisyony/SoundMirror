#include "main.h"


int main() {
    int hr = OK;

    Presenter *presenter = new Presenter();
    presenter->start();
    delete presenter;

    return hr;
}
