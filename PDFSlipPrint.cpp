/*
 * PDFSlipPrint v1.0	- A simple command-line tool to print PDF files using Windows printers.
 * Supports custom page size slip printing.
 * Requires PDFium to compile.
 * Copyright (c) 2025 ZhiCheng Xu
 * 
 * License: MIT
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 */
#include <windows.h>
#include <winspool.h>
#include <fpdfview.h>
#include <fpdf_edit.h>
#include <fpdf_transformpage.h>
#include <string>
#include <iostream>
#include <vector>
#include <iomanip>

struct PrinterMargins {
    int left, top, right, bottom;     // Margins in pixels
    double leftMM, topMM, rightMM, bottomMM; // Margins in millimeters
};

#include <windows.h>
#include <iostream>
#include <vector>

// MultiByte (srcCodePage) -> WideChar (UTF-16)
std::wstring MultiToWide(const std::string &input, UINT srcCodePage) {
    int wideLen = MultiByteToWideChar(srcCodePage, 0, input.c_str(), -1, nullptr, 0);
    if (wideLen <= 0) return L"";
    std::vector<wchar_t> wideStr(wideLen);
    MultiByteToWideChar(srcCodePage, 0, input.c_str(), -1, wideStr.data(), wideLen);
    return std::wstring(wideStr.data());
}

// WideChar (UTF-16) -> MultiByte (dstCodePage)
std::string WideToMulti(const std::wstring &input, UINT dstCodePage) {
    int destLen = WideCharToMultiByte(dstCodePage, 0, input.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (destLen <= 0) return "";
    std::vector<char> destStr(destLen);
    WideCharToMultiByte(dstCodePage, 0, input.c_str(), -1, destStr.data(), destLen, nullptr, nullptr);
    return std::string(destStr.data());
}

// 通用轉碼函式：從 srcCodePage 轉到 dstCodePage
std::string ConvertEncoding(const std::string &input, UINT srcCodePage, UINT dstCodePage) {
    return WideToMulti(MultiToWide(input, srcCodePage), dstCodePage);
}

// 轉換 ANSI (ACP) → UTF-8
std::string AnsiToUtf8(const std::string &ansiStr) {
    return ConvertEncoding(ansiStr, GetACP(), CP_UTF8);
}

// 轉換 UTF-8 → ANSI (ACP)
std::string Utf8ToAnsi(const std::string &utf8Str) {
    return ConvertEncoding(utf8Str, CP_UTF8, GetACP());
}

// 轉換 Console (chcp 設定) → ANSI (ACP)
std::string ConsoleToAnsi(const std::string &consoleStr) {
    return ConvertEncoding(consoleStr, GetConsoleCP(), GetACP());
}

// 轉換 ANSI (ACP) → Console (chcp 設定)
std::string AnsiToConsole(const std::string &ansiStr) {
    return ConvertEncoding(ansiStr, GetACP(), GetConsoleOutputCP());
}

// 轉換 Console (chcp 設定) → UTF-8
std::string ConsoleToUtf8(const std::string &consoleStr) {
    return ConvertEncoding(consoleStr, GetConsoleCP(), CP_UTF8);
}

// 轉換 UTF-8 → Console (chcp 設定)
std::string Utf8ToConsole(const std::string &utf8Str) {
    return ConvertEncoding(utf8Str, CP_UTF8, GetConsoleOutputCP());
}



// Function to get the default printer name
std::wstring GetDefaultPrinterName() {
    DWORD needed;
    //GetDefaultPrinter(NULL, &needed);
     // Use the wide version to get the required buffer size
    GetDefaultPrinterW(NULL, &needed); 
    if (needed == 0) {
        std::cerr << "Failed to get default printer size!" << std::endl;
        return L"";
    }

    //char* printerName = new char[needed];
    wchar_t printerNameWC[needed];
    //if (!GetDefaultPrinter(printerName, &needed)) {
	if (!GetDefaultPrinterW(printerNameWC, &needed)) { 	
        std::cerr << "Failed to get default printer name!" << std::endl;
        //delete[] printerNameWC;
        return L"";
    }

    //std::string result(printerName);
    //delete[] printerName;
   // return result;   
    return std::wstring(printerNameWC);
}


PrinterMargins GetPrinterMargins(HDC hdcPrinter) {
    PrinterMargins margins;

    int dpiX = GetDeviceCaps(hdcPrinter, LOGPIXELSX);  // Horizontal DPI
    int dpiY = GetDeviceCaps(hdcPrinter, LOGPIXELSY);  // Vertical DPI

    int paperWidth = GetDeviceCaps(hdcPrinter, PHYSICALWIDTH);   // Total paper width (pixels)
    int paperHeight = GetDeviceCaps(hdcPrinter, PHYSICALHEIGHT); // Total paper height (pixels)
    int printWidth = GetDeviceCaps(hdcPrinter, HORZRES);         // Printable width (pixels)
    int printHeight = GetDeviceCaps(hdcPrinter, VERTRES);        // Printable height (pixels)
    int marginLeft = GetDeviceCaps(hdcPrinter, PHYSICALOFFSETX); // Left margin (pixels)
    int marginTop = GetDeviceCaps(hdcPrinter, PHYSICALOFFSETY);  // Top margin (pixels)

    // Calculate right and bottom margins in pixels
    int marginRight = paperWidth - (printWidth + marginLeft);
    int marginBottom = paperHeight - (printHeight + marginTop);

    // Convert pixels to millimeters
    double marginLeftMM = (double)marginLeft * 25.4 / dpiX ;
    double marginTopMM = (double)marginTop * 25.4 / dpiY ;
    double marginRightMM = (double)marginRight * 25.4 / dpiX ;
    double marginBottomMM = (double)marginBottom * 25.4/ dpiY;

    // Store values in struct
    margins.left = marginLeft;
    margins.top = marginTop;
    margins.right = marginRight;
    margins.bottom = marginBottom;

    margins.leftMM = marginLeftMM;
    margins.topMM = marginTopMM;
    margins.rightMM = marginRightMM;
    margins.bottomMM = marginBottomMM;

    // Print margin details
    std::cout << "Paper Size: " << (double)paperWidth*25.4/dpiX << " x " << (double)paperHeight*25.4/dpiY << " mm" << std::endl;
    std::cout << "Printable Area: " << (double)printWidth*25.4/dpiX  << " x " << (double)printHeight*25.4/dpiY << " mm" << std::endl;
    std::cout << "DPI: " << dpiX << " x " << dpiY << std::endl;
    std::cout << "Margins (pixels): Left=" << marginLeft << ", Top=" << marginTop 
              << ", Right=" << marginRight << ", Bottom=" << marginBottom << std::endl;
    std::cout << "Margins (mm): Left=" << marginLeftMM << " mm, Top=" << marginTopMM 
              << " mm, Right=" << marginRightMM << " mm, Bottom=" << marginBottomMM << " mm" << std::endl;

    return margins;
}



void SetCustomPaperSize(DEVMODEW* pDevMode, FPDF_PAGE page) {    
    std::cout << "DevMode" << static_cast<void*>(pDevMode) << " page " << static_cast<void*>(page) << std::endl;
    if (!pDevMode || !page) return;
    
	// Convert PDF page dimensions from points to tenths of millimeters
	short pdfWidthTenthMM = (FPDF_GetPageWidth(page)* 10 * 25.4) / 72.0 ;
	short pdfHeightTenthMM = (FPDF_GetPageHeight(page)* 10 * 25.4) / 72.0 ;

	// Assign to DEVMODE
	// Define A4 size in tenths of millimeters
	const short A4_WIDTH_TENTH_MM = 2100;
	const short A4_HEIGHT_TENTH_MM = 2970;

	// Set paper size if portrait or landscape about A4 size use A4 
	if (std::abs(pdfWidthTenthMM - A4_WIDTH_TENTH_MM) + std::abs(pdfHeightTenthMM - A4_HEIGHT_TENTH_MM)  < 25.0 || 
	    std::abs(pdfWidthTenthMM - A4_HEIGHT_TENTH_MM) + std::abs(pdfHeightTenthMM - A4_WIDTH_TENTH_MM) < 25.0) {
	    pDevMode->dmPaperSize = DMPAPER_A4; // Use standard A4 size
    } else {
		pDevMode->dmPaperSize = DMPAPER_USER; // Custom paper size
		//pDevMode->dmPaperWidth = static_cast<short>(pdfWidthTenthMM);
		//pDevMode->dmPaperLength = static_cast<short>(pdfHeightTenthMM);
		pDevMode->dmFields |= DM_PAPERWIDTH | DM_PAPERLENGTH;
	    // Set custom paper size (113mm x 195mm in tenths of millimeters)
	    pDevMode->dmPaperSize = DMPAPER_USER; // Custom paper size
	    pDevMode->dmPaperWidth = pdfWidthTenthMM > pdfHeightTenthMM ? pdfHeightTenthMM : pdfWidthTenthMM;  // 寬大於長則對調
	    pDevMode->dmPaperLength = pdfWidthTenthMM > pdfHeightTenthMM ? pdfWidthTenthMM : pdfHeightTenthMM; // 寬大於長則對調
	    pDevMode->dmFields |= DM_PAPERWIDTH | DM_PAPERLENGTH ;  
    }
    pDevMode->dmOrientation = pdfWidthTenthMM > pdfHeightTenthMM ? DMORIENT_LANDSCAPE : DMORIENT_PORTRAIT; //版面
	pDevMode->dmFields |= DM_ORIENTATION; // Apply orientation setting
}


// Function to create a printer device context (HDC) with custom settings
HDC CreateCustomDCW(const wchar_t* printerName, FPDF_PAGE page) {
    HANDLE hPrinter;

    // Open the printer
    if (!OpenPrinterW((LPWSTR)printerName, &hPrinter, NULL)) {
        std::cerr << "Failed to open printer! Error: " << GetLastError() << std::endl;
        return NULL;
    }

    // Step 1: Get the size of the DEVMODE structure
    DWORD dwNeeded = DocumentPropertiesW(NULL, hPrinter, (LPWSTR)printerName, NULL, NULL, 0);
    if (dwNeeded <= 0) {
        std::cerr << "Failed to get DEVMODE size. Error: " << GetLastError() << std::endl;
        ClosePrinter(hPrinter);
        return NULL;
    }

    // Step 2: Allocate memory for the DEVMODE structure
    DEVMODEW* pDevMode = (DEVMODEW*)malloc(dwNeeded);
    if (!pDevMode) {
        std::cerr << "Memory allocation for DEVMODE failed!" << std::endl;
        ClosePrinter(hPrinter);
        return NULL;
    }

    // Step 3: Retrieve the DEVMODE structure
    if (DocumentPropertiesW(NULL, hPrinter, (LPWSTR)printerName, pDevMode, NULL, DM_OUT_BUFFER) != IDOK) {
        std::cerr << "Failed to retrieve DEVMODE. Error: " << GetLastError() << std::endl;
        free(pDevMode);
        ClosePrinter(hPrinter);
        return NULL;
    }

    // Step 4: Set custom paper size using the PDF page dimensions
    SetCustomPaperSize(pDevMode, page);

    // Step 5: Create printer DC with modified DEVMODE
    HDC hdcPrinter = CreateDCW(NULL, printerName, NULL, pDevMode);
    if (!hdcPrinter) {
        std::cerr << "Failed to create printer device context! Error: " << GetLastError() << std::endl;
        free(pDevMode);
        ClosePrinter(hPrinter);
        return NULL;
    }

    // Cleanup
    free(pDevMode);
    ClosePrinter(hPrinter);

    return hdcPrinter;
}


void PrintPDF(const wchar_t* printerName, const char* pdfPath) {
	//std::cout << "Initialize PDFium" << std::endl;
    // Initialize PDFium
    FPDF_InitLibrary();

    // Load PDF Document
    
    FPDF_DOCUMENT doc = FPDF_LoadDocument(pdfPath, nullptr);
    if (!doc) {
        std::cerr << "Failed to load PDF file!" << std::endl;
        return;
    }
	
	 // Load the first page to determine the paper size
    FPDF_PAGE firstPage = FPDF_LoadPage(doc, 0);
    if (!firstPage) {
        std::cerr << "Failed to load the first PDF page!" << std::endl;
        FPDF_CloseDocument(doc);
        return;
    }

	
	
	
    // Create custom DCA for printing
    HDC hdcPrinter = CreateCustomDCW(printerName, firstPage);
    if (hdcPrinter == NULL) {
        std::cerr << "Failed to create printer device context!" << std::endl;
        FPDF_CloseDocument(doc);
        return;
    }
	// Get printer margins
    PrinterMargins margins = GetPrinterMargins(hdcPrinter);
    // Adjust printing based on margins
    int offsetX = -margins.left; // Move content left to compensate for margi
    int offsetY = -margins.top;  // Move content up to compensate for margin
    
    // Get printer DPI (dots per inch)
    int dpiX = GetDeviceCaps(hdcPrinter, LOGPIXELSX);
    int dpiY = GetDeviceCaps(hdcPrinter, LOGPIXELSY);

    // Start printing
    DOCINFOW di = {sizeof(DOCINFOW)};
    di.lpszDocName = L"PDF Print Job";

    if (StartDocW(hdcPrinter, &di) > 0) {
        // Loop through all pages
        int pageCount = FPDF_GetPageCount(doc);
        for (int pageIndex = 0; pageIndex < pageCount; ++pageIndex) {
            // Load each page
            FPDF_PAGE page = FPDF_LoadPage(doc, pageIndex);
            if (!page) {
                std::cerr << "Failed to load PDF page!" << std::endl;
                continue;
            }

            // Get PDF page dimensions (default PDFium units: 1/72 inch)
            double pdfWidth = FPDF_GetPageWidth(page);
            double pdfHeight = FPDF_GetPageHeight(page);

            // Scale PDF dimensions from 72 DPI to printer DPI
            int width = static_cast<int>(pdfWidth * dpiX / 72);
            int height = static_cast<int>(pdfHeight * dpiY / 72);

            // Create a bitmap with the scaled size
            FPDF_BITMAP bitmap = FPDFBitmap_Create(width, height, 0);
            if (!bitmap) {
                std::cerr << "Failed to create PDF bitmap!" << std::endl;
                FPDF_ClosePage(page);
                continue;
            }

            // Render PDF to bitmap
            FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xFFFFFFFF); // White background
            FPDF_RenderPageBitmap(bitmap, page, 0, 0, width, height, 0, FPDF_ANNOT);

            // Get bitmap buffer
            void* buffer = FPDFBitmap_GetBuffer(bitmap);
            int stride = FPDFBitmap_GetStride(bitmap);

            // Prepare BITMAPINFO structure
            BITMAPINFO bmi = {};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = width;
            bmi.bmiHeader.biHeight = -height;  // Top-down DIB
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            if (StartPage(hdcPrinter) > 0) {
                // Scale and print the bitmap to match printer DPI
                StretchDIBits(hdcPrinter, 
                    offsetX, offsetY, width, height, // Destination (printer)
                    0, 0, width, height, // Source (bitmap)
                    buffer, 
                    &bmi, 
                    DIB_RGB_COLORS, SRCCOPY);

                EndPage(hdcPrinter);
            }

            // Cleanup
            FPDFBitmap_Destroy(bitmap);
            FPDF_ClosePage(page);
        }

        EndDoc(hdcPrinter);
    }

    // Cleanup
    DeleteDC(hdcPrinter);
    FPDF_CloseDocument(doc);
    FPDF_DestroyLibrary();

    std::cout << "Document printed successfully." << std::endl;
}

//Windows stores the command-line arguments as UTF-16,
// we can use GetCommandLineW() to retrieve them
std::vector<std::wstring> GetUtf16Args() {
    int argc;
    wchar_t** argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argvW) return {};

    std::vector<std::wstring> args(argvW, argvW + argc);
    LocalFree(argvW);
    return args;
}

int main(int argc, char* argv[]) {
	std::cout << "PDFSlipPrint v1.0 - A simple command-line tool to print PDF files, including custom-sized slips, using Windows printers." << std::endl;
    std::cout << "Copyright (c) 2025 ZhiCheng Xu		License: MIT (WITHOUT ANY WARRANTY)" << std::endl << std::endl;
    if (argc < 2 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " [-d \"Printer Name\"] filename.pdf" << std::endl;
        std::cerr << "Example:" << argv[0] << " -d \"Pantum P2500 Series\" test.pdf" << std::endl;
        return 1;
    }
	UINT consoleCP = GetConsoleCP();
    std::string printerName;
    std::string defaultPrinterName;
    std::string pdfFile;
	std::vector<std::wstring> args = GetUtf16Args();

    if (argc == 4 && std::string(argv[1]) == "-d") {		 
        printerName = WideToMulti(args[2],consoleCP);;
        pdfFile = WideToMulti(args[3],consoleCP);
        std::cout << "Manually set printer" << std::endl;
    } else {
        pdfFile = WideToMulti(args[1],consoleCP);
        std::cout << "Use default printer"  << std::endl;
    }
	defaultPrinterName= WideToMulti(GetDefaultPrinterName(),consoleCP);
    if (printerName.empty()) {
        //printerName = AnsiToConsole(GetDefaultPrinterName()); //dosen't work for utf8
        printerName = WideToMulti(GetDefaultPrinterName(),consoleCP);
        if (printerName.empty()) {
            std::cerr << "No default printer found!" << std::endl;
            return 1;
        }
    }

    std::cout << "Using printer: " << printerName << std::endl;
    std::cout << "Printing file: " << pdfFile << std::endl;

    // Call PrintPDF function with resolved printer name and PDF file
    // Convert the filename before passing it to PDFium
    std::string pdfFileUtf8 = ConsoleToUtf8(pdfFile);
    //std::string acpPrinterName = ConsoleToAnsi(printerName);
    
    UINT acp = GetACP();
    std::cout << "ANSI Code Page (ACP): " << acp << std::endl;
    
    UINT consoleOutputCP = GetConsoleOutputCP();
    std::cout << "Console Input Code Page (chcp): " << consoleCP << std::endl;
    std::cout << "Console Output Code Page (chcp): " << consoleOutputCP << std::endl;
    
    std::wstring printerNameW=MultiToWide(printerName,consoleCP);
    std::cout << "pdfFileUtf8: " << pdfFileUtf8 << std::endl;
    const wchar_t* mutablePrinterName(printerNameW.c_str());
    //mutablePrinterName.push_back(L'\0');
    
    std::cout << "Selected printer name (hex): ";
	// Reinterpret the wide string as a byte array
    const char* bytes = reinterpret_cast<const char*>(mutablePrinterName);
    // Calculate the number of bytes (excluding null terminator)
    size_t byteCount = wcslen(mutablePrinterName) * sizeof(wchar_t);
    // Print each byte in hex
    for (size_t i = 0; i < byteCount; ++i) {
        std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)bytes[i] << " ";
    }
    
    
	std::wstring printerDefaultNameW=MultiToWide(defaultPrinterName,consoleCP);
	const wchar_t* mutableDefaultPrinterName(printerDefaultNameW.c_str());
	std::cout << "Default printer name (hex): ";
	// Reinterpret the wide string as a byte array
    bytes = reinterpret_cast<const char*>(mutableDefaultPrinterName);
    // Calculate the number of bytes (excluding null terminator)
    byteCount = wcslen(mutableDefaultPrinterName) * sizeof(wchar_t);
    // Print each byte in hex
    for (size_t i = 0; i < byteCount; ++i) {
        std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)(unsigned char)bytes[i] << " ";
    }
	if (mutableDefaultPrinterName==mutablePrinterName){
		std::cout << "The printer you selected is the same as default one."  << std::endl;
	} else {
		std::cout << "The printer you selected is the different from default one."  << std::endl;
	}		
	
	PrintPDF( mutablePrinterName, pdfFileUtf8.c_str());
    return 0;
}




