#include "main.h"

void SaveBitmap(HBITMAP hBitmap, HDC hMemoryDC, const std::wstring& filePath);

#define SLEEP_TIMER 2000

int main() {
	Sleep(SLEEP_TIMER); // Sleep for 2 seconds to give the user time to switch to the window they want to screenshot
    // Get the desktop device context
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    // Get the width and height of the screen
    int screenWidth = GetDeviceCaps(hScreenDC, HORZRES);
    int screenHeight = GetDeviceCaps(hScreenDC, VERTRES);

    // Create a compatible bitmap from the screen device context
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, screenWidth, screenHeight);

    // Select the compatible bitmap into the memory device context
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemoryDC, hBitmap);

    // Bit block transfer into our compatible memory DC
    BitBlt(hMemoryDC, 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0, SRCCOPY);

    // Restore the old bitmap
    SelectObject(hMemoryDC, hOldBitmap);

    // Save the bitmap to the desktop
    wchar_t* userProfile = nullptr;
    size_t len = 0;
    if (_wdupenv_s(&userProfile, &len, L"USERPROFILE") == 0 && userProfile != nullptr) {
        std::wstring desktopPath = std::wstring(userProfile) + L"\\Desktop\\screenshot.bmp";
        free(userProfile);
        SaveBitmap(hBitmap, hMemoryDC, desktopPath);
    } else {
        std::wcerr << L"Failed to get USERPROFILE environment variable." << std::endl;
    }

    // Clean up
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);

    std::wstring desktopPath = L"C:\\Users\\manor\\Desktop\\screenshot.bmp";


    std::wcout << L"Screenshot saved to " << desktopPath << std::endl;

    return 0;
}

void SaveBitmap(HBITMAP hBitmap, HDC hMemoryDC, const std::wstring& filePath) {
    BITMAP bmp;
    PBITMAPINFO pbmi;
    WORD cClrBits;
    HANDLE hf;                 // file handle
    BITMAPFILEHEADER hdr;       // bitmap file-header
    PBITMAPINFOHEADER pbih;     // bitmap info-header
    LPBYTE lpBits;              // memory pointer

    // Get the bitmap information
    if (!GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bmp)) {
        std::cerr << "GetObject failed" << std::endl;
        return;
    }

    cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
    if (cClrBits == 1) {
        cClrBits = 1;
    } else if (cClrBits <= 4) {
        cClrBits = 4;
    } else if (cClrBits <= 8) {
        cClrBits = 8;
    } else if (cClrBits <= 16) {
        cClrBits = 16;
    } else if (cClrBits <= 24) {
        cClrBits = 24;
    } else {
        cClrBits = 32;
    }

    if (cClrBits != 24) {
        pbmi = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * static_cast<size_t>(1 << cClrBits));
    } else {
        pbmi = (PBITMAPINFO)LocalAlloc(LPTR, sizeof(BITMAPINFOHEADER));
    }

    if (pbmi == nullptr) {
        std::cerr << "LocalAlloc failed" << std::endl;
        return;
    }

    pbih = (PBITMAPINFOHEADER)pbmi;
    if (pbih != nullptr) {
        pbih->biSize = sizeof(BITMAPINFOHEADER);
        pbih->biWidth = bmp.bmWidth;
        pbih->biHeight = bmp.bmHeight;
        pbih->biPlanes = bmp.bmPlanes;
        pbih->biBitCount = bmp.bmBitsPixel;
        if (cClrBits < 24) {
            pbih->biClrUsed = (1 << cClrBits);
        }
        pbih->biCompression = BI_RGB;
        pbih->biSizeImage = ((pbih->biWidth * cClrBits + 31) & ~31) / 8 * pbih->biHeight;
        pbih->biClrImportant = 0;
    } else {
        std::cerr << "pbih is null" << std::endl;
        LocalFree(pbmi);
        return;
    }

    lpBits = (LPBYTE)GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);
    if (lpBits == nullptr) {
        std::cerr << "GlobalAlloc failed" << std::endl;
        LocalFree(pbmi);
        return;
    }

    if (GetDIBits(hMemoryDC, hBitmap, 0, (WORD)pbih->biHeight, lpBits, pbmi, DIB_RGB_COLORS) == 0) {
        std::cerr << "GetDIBits failed" << std::endl;
        GlobalFree(lpBits);
        LocalFree(pbmi);
        return;
    }

    hf = CreateFile(filePath.c_str(), GENERIC_READ | GENERIC_WRITE, (DWORD)0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, (HANDLE)NULL);
    if (hf == INVALID_HANDLE_VALUE) {
        std::cerr << "CreateFile failed" << std::endl;
        GlobalFree(lpBits);
        LocalFree(pbmi);
        return;
    }

    hdr.bfType = 0x4d42; // "BM"
    hdr.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof(RGBQUAD) + pbih->biSizeImage);
    hdr.bfReserved1 = 0;
    hdr.bfReserved2 = 0;
    hdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + pbih->biSize + pbih->biClrUsed * sizeof(RGBQUAD);

    DWORD dwTmp;
    if (!WriteFile(hf, (LPVOID)&hdr, sizeof(BITMAPFILEHEADER), (LPDWORD)&dwTmp, NULL)) {
        std::cerr << "WriteFile failed" << std::endl;
        CloseHandle(hf);
        GlobalFree(lpBits);
        LocalFree(pbmi);
        return;
    }

    if (!WriteFile(hf, (LPVOID)pbih, sizeof(BITMAPINFOHEADER) + pbih->biClrUsed * sizeof(RGBQUAD), (LPDWORD)&dwTmp, NULL)) {
        std::cerr << "WriteFile failed" << std::endl;
        CloseHandle(hf);
        GlobalFree(lpBits);
        LocalFree(pbmi);
        return;
    }

    if (!WriteFile(hf, (LPSTR)lpBits, (int)pbih->biSizeImage, (LPDWORD)&dwTmp, NULL)) {
        std::cerr << "WriteFile failed" << std::endl;
        CloseHandle(hf);
        GlobalFree(lpBits);
        LocalFree(pbmi);
        return;
    }

    if (!CloseHandle(hf)) {
        std::cerr << "CloseHandle failed" << std::endl;
    }

    GlobalFree(lpBits);
    LocalFree(pbmi);
}
