// InjectionLibrary.cpp : Defines the exported functions for the DLL application.
//

#include "InjectionLibrary.h"
#include <windows.h>
#include <string>

INJECTIONLIBRARY_API void library_function1() {
	auto process_id = GetCurrentProcessId();
	auto message = std::string("This is library_function1(), running from process ") + std::to_string(process_id);

	MessageBox(
		NULL,
		message.c_str(),
		"DLL Injection Successful",
		MB_ICONEXCLAMATION);
}

INJECTIONLIBRARY_API void library_function2(char* s) {
	auto process_id = GetCurrentProcessId();
	auto message = std::string("This is library_function2(). Called with parameter: ") + std::string(s);

	MessageBox(
		NULL,
		message.c_str(),
		"DLL Injection Successful",
		MB_ICONEXCLAMATION);
}

