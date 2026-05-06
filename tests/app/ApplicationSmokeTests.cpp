#include <type_traits>

#include "app/MainWindow.h"

int main() {
    static_assert(std::is_base_of_v<QMainWindow, MainWindow>);
    return 0;
}
