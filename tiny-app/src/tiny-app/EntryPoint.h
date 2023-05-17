#pragma once


extern tiny::IApplication* tiny::CreateApplication();

int main(int argc, char** argv)
{
    std::unique_ptr<tiny::IApplication> app = std::unique_ptr<tiny::IApplication>(tiny::CreateApplication());
    app->Run();

    return 0;
}