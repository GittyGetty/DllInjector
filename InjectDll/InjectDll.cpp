#include <cstdio>
#include <string>
#include <iostream>
#include <algorithm>
#include <functional>

#include <windows.h>
#include <tlhelp32.h>

using std::string;

/**
 * Lowercase a C++ string for case insensitive comparison.
 */
void to_lower(std::string &s) {
	std::transform(s.begin(), s.end(), s.begin(), ::tolower);
}

/**
 * If this example is to be expanded, it might be better to move those declarations
 * into a header file. Documentation follows with their defintion.
 */
class DllInjector;
class Functor0Arg;
template<typename T> class Functor1Arg;

class Functor {
public:
	Functor(DllInjector *i, string library, string procedure);
	template<typename T> operator T();

protected:
	DllInjector *imp_;
	string library_, procedure_;
};

class Functor0Arg : public Functor {
public:
	Functor0Arg(DllInjector* i, string library, string procedure);
	int operator()();
};

template<typename T>
class Functor1Arg : public Functor {
public:
	Functor1Arg(DllInjector* i, string library, string procedure);
	int operator()(T argument, size_t size);
};

class DllInjector {
public:
	DllInjector(string process_name);
	~DllInjector();
	template <typename T>
	Functor1Arg<T> get_procedure(string library, string procedure);
	Functor0Arg get_procedure(string library, string procedure);
	int call_procedure(const char*, const char*, void* = NULL, size_t = 0);

private:
	string process_name_;
	HANDLE process_handle_ = NULL;

	void load_library(string library);
	void get_process();
};

/**
* This class is an attempt at type safety for calling library procedures with
* different number of arguments. It enables defining the type of arguments
* for a remotely called procedure.
*/
Functor::Functor(DllInjector *i, string library, string procedure) {
	imp_ = i;
	library_ = library;
	procedure_ = procedure;
}

Functor0Arg::Functor0Arg(DllInjector* i, string library, string procedure)
	: Functor(i, library, procedure) {}

int Functor0Arg::operator()() {
	return imp_->call_procedure(library_.c_str(), procedure_.c_str());
}

template<typename T>
Functor1Arg<T>::Functor1Arg(DllInjector* i, string library, string procedure)
	: Functor(i, library, procedure) {}

template<typename T>
int Functor1Arg<T>::operator()(T argument, size_t size) {
	return imp_->call_procedure(library_.c_str(), procedure_.c_str(), (void*)argument, size);
}

/**
 * Class providing run-time, remote DLL injection. It can make another
 * process load a library and run exported procedures from it. The called
 * procedures are executed in the context of the other process.
 *
 * See the main method in this file for a usage example.
 *
 * This is intended as a skeleton. This code has been written for the
 * interest of the author. I am sharing this code because I would like to
 * give back to the communities that I have drawn information from.
 *
 * I have also tried to provide a clean interface that only requires the
 * minimum input possible. The interface should encourage the use of C++
 * concepts and hide any details that depend on the exact underlying
 * mechanics of the invocation. Many tutorials and examples I have come across
 * for this kind of functionality, especially the ones on sites dedicated
 * to "game hacking", seemed very intransparent to me, with many hardcoded
 * constants, little context for their semantics, badly well-written
 * documentation lacking basic sentence structure, and sometimes contradictory
 * or incorrect explanations. I do not mean to be critical, but I felt there
 * was a gap to fill.
 *
 * Issues:
 * - Error handling is mostly absent.
 * - There is no logging. If any step fails, the user will have to debug
 *   this code to determine its cause.
 * - It could be possible to generate prototypes for the library procedures
 *   to be called and forward those to the DLL injector. That way, one could
 *   call procedures on the remote process with the same syntax and type safety
 *   as calling local procedures through the regular C/C++ invocation syntax.
 *   This could make the interface even easier to use.
 */
/**
 * process_name: name of the process, with ".exe" suffix if present,
 *               to inject into. Comparison will be case insensitive.
 */
DllInjector::DllInjector(string process_name) {
	process_name_ = process_name;
	to_lower(process_name_);
	get_process();
}

DllInjector::~DllInjector() {
	if (process_handle_ != NULL)
		CloseHandle(process_handle_);
}

/**
 * Call a procedure from a library by name, passing the given arguments.
 * I have chosen to use the invocation operator to make the syntax look more
 * like a direct function invocation.
 *
 * To make calling the remote procedure even simpler, one could return a
 * function pointer instead, set to the instance of a class  method that will
 * transparently invoke the procedure remotely.
 *
 * library: The full path to the library from which to call the procedure.
 * procedure: The name of the procedure. This is currently assumed to be the
 *            name of the procedure as exported by the DLL. If name mangling
 *            has been applied to the original procedure, this name must reflect
 *            that full, mangled name exactly as it appears in the library. For
 *            C libraries and DLL exports declared with extern "C" this will
 *            be the plain, original name of the procedure.
 * argument: The address of the argument to pass to the procedure. Can be NULL
 *           if the procedure takes no arguments.
 * argument_size: The size of the data pointed to by argument.
 */
template<typename T>
Functor1Arg<T> DllInjector::get_procedure(string library, string procedure) {
	load_library(library);
	return Functor1Arg<T>(this, library, procedure);
}

/**
 * Overload for a procedure with no arguments.
 */
Functor0Arg DllInjector::get_procedure(string library, string procedure) {
	load_library(library);
	return Functor0Arg(this, library, procedure);
}

/**
 * Call a procedure from a library in the process this DLL Injector instance
 * was constructed for.
 * library: The name of the library. Can be an absolute or relative path. Use 
 *          a relative path if the library is located both in the local and
 *          remote process's library search paths. Use an absolute path otherwise.
 * procedure: The name of the procedure to call, literally as it appears in the
 *            exports of the library.
 * argument: Address of the procedures argument. Due to the signature of the Windows
 *           API used, this currently only supports one argument. This is not a
 *           limitation on how much data one can pass, though, because one can simply
 *           define a struct containing as many parameters as dessired, for example.
 * argument_size: The size of the data passed in the memory pointed to by argument.
 */
int DllInjector::call_procedure(
		const char* library,
		const char* procedure,
		void* argument,
		size_t argument_size) {
	// TODO: Add error handling and formatting. Code for this can be found here:
    // http://msdn.microsoft.com/en-us/library/ms680582%28VS.85%29.aspx
	auto module = GetModuleHandle(library);

	// Address returned is from this process but also valid for the remote process, 
	// so there is no issue in passing it to CreateRemoteThread below.
	auto address = (LPTHREAD_START_ROUTINE)GetProcAddress(module, procedure);
	auto flags = MEM_RESERVE | MEM_COMMIT;
	auto memory = VirtualAllocEx(process_handle_, NULL, argument_size, flags, PAGE_READWRITE);

	WriteProcessMemory(process_handle_, memory, argument, argument_size, NULL);
	return (int)CreateRemoteThread(process_handle_, NULL, 0, address, memory, NULL, NULL);
}

/**
 * Load the given library into this and the remote process. The only reason to load it is
 * to be able to gain a handle on it in the local process and locate exports. If these
 * addresses were known in advance, there would be no need to load it into the local process.
 */
void DllInjector::load_library(string library) {
	LoadLibrary(library.c_str());
	call_procedure("kernel32.dll", "LoadLibraryA", (void*)library.c_str(), library.size());
}

/**
 * Mostly for convenience. There are ways with less code that would have served equally
 * well for demonstration purposes on how to inject a library, such as opening a process
 * directly by its process ID (PID), but changing the PID in code every time the target
 * process restarts was increasingly annoying over time.
 * I made this method static so it's easier to move it out of this class if desired.
 */
void DllInjector::get_process() {
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	PROCESSENTRY32 entry;
	BOOL has_process = Process32First(snapshot, &entry);

	while (has_process)	{
		string current_process(entry.szExeFile);
		to_lower(current_process);

		if (process_name_ == current_process)
			process_handle_ = OpenProcess(PROCESS_ALL_ACCESS, false, entry.th32ProcessID);
		has_process = Process32Next(snapshot, &entry);
	}
	CloseHandle(snapshot);
}

/**
 * Usage example. To run this example, open an instance of notepad under Windows. 
 * Compile the library that should have come with this example or create your own
 * and replace the exports' names below. Using any other DLL should work fine, too.
 * Make sure the path to the library matches the one on your system. Make sure
 * that the architecture type, e.g. x86 or x64 AKA 32bit vs 64bit, matches between 
 * the library, this injector, and the target process. The task manager under 64 bit 
 * Windows will usually append the suffix "*32" to the process's name if it is a 32 
 * bit process. If there is a mismatch, re-target your compiler to produce code 
 * matching the target process's architecture and recompile both the library and 
 * this injector. Using Visual Studio 2013, this code has been tested both against
 * 32bit and 64bit Windows 7. 
 *
 * I have not tested this code much beyond running code from my own library against
 * small applications like notepad. If I want to do code injection, I would probably 
 * use one of the existing frameworks that have received far more development time 
 * and are far wider in scope. I have written this code to learn from it.
 *
 * Proper error handling is lacking at this point. If you would like to investigate
 * issues with this code, set breakpoints in the get_procedure and call_procedure
 * methods and check that all handlers are valid. The Windows API "GetLastError()"
 * will provide further details on root causes. 
 * GetLastError():
 * http://msdn.microsoft.com/en-us/library/windows/desktop/ms679360(v=vs.85).aspx
 * Ideally, this code should be improved to check all return values of all Windows 
 * APIs used and errors properly formatted and localized using 
 * http://msdn.microsoft.com/en-us/library/windows/desktop/ms679351(v=vs.85).aspx
 * I have not done so, because I did not want to write any code much beyond what
 * I was interested in tinkering with at the time I wrote this.
 */
void main() {
	// Name of the remote process to execute the library's procedures in.
	DllInjector injector("notepad.exe");

	// Library to run the procedures from.
	string library = "E:\\InjectionLibrary\\x64\\Debug\\InjectionLibrary.dll";

	// Find procedures to execute in the library. The returned objects are "functors"
	// or "function objects", so syntactically they can be called like a regular 
	// function and potentially passed into STL methods, too, when it makes sense.
	auto library_function1 = injector.get_procedure(library, "library_function1");
	auto library_function2 = injector.get_procedure<const char*>(library, "library_function2");

	// Call the first function. Notice that the syntax is indistinguishable from a regular
	// function call.
	library_function1();

	// Passing arguments requires specifying the size of the memory backing it.
	string message = "DLL Injector calling.";
	library_function2(message.c_str(), message.size());

	// If you don't like this, you can hide the determination of the argument size.
	auto fn = [&library_function2](string s){ library_function2(s.c_str(), s.size()); };

	// Now, passing C++ objects to this function is just as convenient as a local function call.
	fn("Calling again");
}