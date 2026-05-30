// MagnifyShit 2.0 entry point. All lifecycle lives in magshit::app::Application.

#include "app/Application.hpp"

#include <Windows.h>

/**
 * @brief Unicode Windows application entry point.
 * @param hInst Module instance for this process.
 * @param nCmdShow Initial show command forwarded to the main window.
 * @return Process exit code returned by `magshit::app::Application`.
 */
int APIENTRY wWinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nCmdShow)
{
    magshit::app::Application app;
    return app.run(hInst, nCmdShow);
}
