#pragma once


extern tiny::IApplication* tiny::CreateApplication();

int main(int argc, char** argv)
{
    try
    {
        std::unique_ptr<tiny::IApplication> app = std::unique_ptr<tiny::IApplication>(tiny::CreateApplication());
        app->Run();
    }
    catch (const tiny::TinyAppException& e)
    {
        MessageBox(nullptr, e.what(), e.GetType(), MB_OK | MB_ICONEXCLAMATION);
    }
    catch (const std::exception& e)
    {
        MessageBox(nullptr, e.what(), "Standard Exception", MB_OK | MB_ICONEXCLAMATION);
    }
    catch (...)
    {
        MessageBox(nullptr, "No details available", "Unknown Exception", MB_OK | MB_ICONEXCLAMATION);
    }

    return 0;
}