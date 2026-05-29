// MagnifyShit 2.0 entry point. All lifecycle lives in magshit::app::Application.

#include "app/Application.hpp"

#include <Windows.h>

int APIENTRY wWinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nCmdShow)
{
    magshit::app::Application app;
    return app.run(hInst, nCmdShow);
}
