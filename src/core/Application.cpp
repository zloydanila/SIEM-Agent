#include "Application.h"

Application::Application() {}

Application* Application::instance(){

    static Application instance;
    return &instance;
}