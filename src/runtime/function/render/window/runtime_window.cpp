#include "runtime_window.h"

namespace Meow
{
    RuntimeWindow::RuntimeWindow(std::size_t id)
        : Window::Window(id)
    {}

    RuntimeWindow::~RuntimeWindow() {}

    void RuntimeWindow::Tick(float dt) { Window::Tick(dt); }
} // namespace Meow
