#include <windows.h>
#include <iostream>
#include "GetGDIObjects.h"

static const char* const gdiObjectNames[] =
{
    "PEN",
    "BRUSH",
    "DC",
    "METADC",
    "PAL",
    "FONT",
    "BITMAP",
    "REGION",
    "METAFILE",
    "MEMDC",
    "EXTPEN",
    "ENHMETADC",
    "ENHMETAFILE",
    "COLORSPACE"
};

int main()
{
    while (true)
    {
        auto const gdiObject = CreateDCW(L"DISPLAY", nullptr, nullptr, nullptr);

        std::cout << "Created GDI object handle: " << std::hex << gdiObject << std::endl;

        std::cout << "GDI objects: " << std::endl;

        for (auto const gdiObject : GetGDIObjects())
        {
            std::cout << "GDI object handle: " << std::hex << gdiObject << ", type: " << gdiObjectNames[GetObjectType(gdiObject) - 1] << std::endl;
        }

        std::cout << "Finished" << std::endl;

        Sleep(500);
    }
}
