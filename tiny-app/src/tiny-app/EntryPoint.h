#pragma once


extern tiny::Application* tiny::CreateApplication();

int main(int argc, char** argv)
{
    std::unique_ptr<tiny::Application> app = std::unique_ptr<tiny::Application>(tiny::CreateApplication());
    app->Run();

    return 0;
}