/*
* Paul Mehta 2012-10-10
* paul@paulmehta.com
*/

#include "pin.H"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <process.h>
#include <sstream>

/* ===================================================================== */
// Command line switches
/* ===================================================================== */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "output", "console", "Specify a file name for output. Enter \".\" (without the quotes) to auto-generate the filename. If not specified, console is used.");
KNOB<string> KnobModule(KNOB_MODE_WRITEONCE, "pintool", "module", "", "Specify the module to instrument (without file extension). Ex. -module kernel32");
KNOB<bool> KnobNoSymbols(KNOB_MODE_WRITEONCE, "pintool", "no_symbols", "false", "Do not Load Symbols.");
KNOB<bool> KnobNoResolveVirtualCalls(KNOB_MODE_WRITEONCE, "pintool", "no_resolve_virtual_calls", "false", "Don't resolve indirect calls.");
KNOB<bool> KnobNoTrace(KNOB_MODE_WRITEONCE, "pintool", "no_trace", "false", "Don't trace basic blocks.");
KNOB<bool> KnobAppend(KNOB_MODE_WRITEONCE, "pintool", "append", "false", "Do not include script header (appending to existing file).");
KNOB<bool> KnobVerbose(KNOB_MODE_WRITEONCE, "pintool", "verbose", "false", "Include additional output as comments.");
KNOB<bool> KnobNoConsole(KNOB_MODE_WRITEONCE, "pintool", "no_console", "false", "Do not output to console.");
KNOB<bool> KnobDeferOutput(KNOB_MODE_WRITEONCE, "pintool", "defer_output", "false", "Defer output till process exit. Otherwise, live output from live process.");
KNOB<string> KnobTraceColor(KNOB_MODE_WRITEONCE, "pintool", "trace_color", "0x7BF0D3", "The initial color (light green) for control flow tracing.");

/* ================================================================== */
// Global variables 
/* ================================================================== */
static string fileout;
static UINT64 resolvedCount = 0;
static std::ostream * out;
static string module;
static ADDRINT imgBaseAddr = 0;
static ADDRINT imgEndAddr = RSIZE_MAX; // 0x7FFFFFFF 32-bit or 0x7FFFFFFF'FFFFFFFF 64-bit
static UINT64 bbcount = 0;

/// <summary>
/// Holds resolved virtual call references
/// </summary>
typedef struct VirtualCall
{
	ADDRINT _caller;
	ADDRINT *_targets;
	int _numTargets;
	struct VirtualCall * _next;
} VirtualCall;

static VirtualCall *virtualCallList = NULL;

typedef struct ModuleEntry
{
	ADDRINT _start;
	ADDRINT _end;
	string _name;
	struct ModuleEntry *_next;
} ModuleEntry;

static ModuleEntry *moduleList = 0;

typedef struct BblTrace
{
	ADDRINT _address;
	bool _marked;
	struct BblTrace * _next;
} BblTrace;
BblTrace * bblTraceList = 0;

/// <summary>
/// Writes s to the output stream. If the output stream is not cout, and the -no_console option was not specified, output s to cout.
/// </summary>
/// <param name="s">The s.</param>
static void output(string s)
{
	// Writes s to the output stream.
	*out << s;

	// If the output stream is not cout, and the -no_console option was not specified, output s to cout.
	if (!KnobNoConsole.Value() && out != &cout)
		cout << s;
}

/// <summary>
/// Dumps all the symbols found for an image.
/// </summary>
/// <param name="img">The img.</param>
void DumpSymbols(IMG img)
{
	std::ostringstream ss;
	ss << setfill('0');

	ss << "# SYMBOLS" << endl;

	for (SYM sym = IMG_RegsymHead(img); SYM_Valid(sym); sym = SYM_Next(sym))
	{
		string symPureName = PIN_UndecorateSymbolName(SYM_Name(sym), UNDECORATION_NAME_ONLY);
		ss << "# SYM:   " << symPureName << endl;
	}

	output(ss.str());
}

static bool VirtualCallListContains(ADDRINT caller)
{
	if (virtualCallList == 0)
		return false;

	for (VirtualCall *vcallList = virtualCallList; vcallList; vcallList = vcallList->_next)
	{
		if (vcallList->_caller == caller)
			return true;
	}
	return false;
}

/// <summary>
/// Converts a string to lower case.
/// </summary>
/// <param name="str">The string.</param>
/// <returns>Lower case string</returns>
static string ToLower(string str)
{
	char c;
	char diff = 'a' - 'A';
	int len = str.length();
	for (int i = 0; i < len; i++)
	{
		c = str[i];
		if (c >= 'A' && c <= 'Z')
			c += diff;
		str[i] = c;
	}
	return str;
}

/// <summary>
/// Strips the path from a filename.
/// </summary>
/// <param name="filename">The filename.</param>
/// <returns>The filename without the path.</returns>
static string StripPath(string filename)
{
	size_t index = filename.find_last_of("/\\");

	if (index >= 0)
		return filename.substr(index + 1, 0x100);
	return filename;
}

/// <summary>
/// Parses the filename without the extension.
/// </summary>
/// <param name="filename">The filename.</param>
/// <returns>Filename without extension.</returns>
static string FilenameWithoutExtension(string filename)
{
	size_t index = filename.find_last_of('.');

	if (index >= 0)
		return filename.substr(0, index);
	return filename;
}

/// <summary>
/// Gets the entry of the module that contains address.
/// </summary>
/// <param name="address">The address.</param>
static ModuleEntry *GetModuleEntry(ADDRINT address)
{
	for (ModuleEntry *mod = moduleList; mod != 0; mod = mod->_next)
	{
		if (mod->_start <= address && mod->_end > address)
			return mod;
	}

	return 0;
}

/// <summary>
/// Invalidates the code cache outside of the desired module.
/// </summary>
static void InvalidateTraceExternal()
{
	UINT32 invalidated = CODECACHE_InvalidateRange(0, imgBaseAddr);
	invalidated += CODECACHE_InvalidateRange(imgEndAddr, RSIZE_MAX);

	if (invalidated > 0 && KnobVerbose.Value())
	{
		std::ostringstream ss;
		ss << setfill('0');
		ss << "# Invalidated " << hex << invalidated << " traces" << endl << flush;
		output(ss.str());
	}
}

/// <summary>
/// Filters the trace by constraining to addresses within the module specified on the command-line.
/// </summary>
/// <param name="trace">The trace.</param>
/// <returns></returns>
static bool FilterTrace(TRACE trace)
{
	if (!imgBaseAddr || !imgEndAddr)
		return false;

	if (TRACE_Address(trace) < imgBaseAddr || TRACE_Address(trace) > imgEndAddr)
		return false;

	return true;
}

/// <summary>
/// Outputs virtual calls as script.
/// </summary>
/// <param name="caller">The caller.</param>
/// <param name="target">The target.</param>
static void OutputVirtualCall(ADDRINT caller, ADDRINT target)
{
	std::ostringstream ss;
	ss << setfill('0');

	if (target >= imgBaseAddr && target < imgEndAddr) // Target is within current module
	{
		ss << "createXRef(0x" << setw(8) << hex << (caller - imgBaseAddr) << ", 0x" << setw(8) << hex << (target - imgBaseAddr) << ")";
		
	}
	else // Target is outside of the current module
	{
	
		ModuleEntry *entry = GetModuleEntry(target);
		ss << "createXRefExternal(0x" << setw(8) << hex << (caller - imgBaseAddr) << ", \"" 
			<< (entry == 0 ? string("__unk__") : entry->_name) 
			<< "!" 
			<< RTN_FindNameByAddress(target) << " "
			<< hex << target << "\")";

		if (KnobVerbose.Value())
			ss << "\t# " << setw(8) << hex << target - entry->_start;
	}

	ss << endl;
	output(ss.str());
}

/// <summary>
/// Prints all resolved virtual calls for a caller.
/// </summary>
/// <param name="vcall">The vcall.</param>
static void OutputResolvedVirtualCall(VirtualCall *vcall)
{
	for (int i = 0; i < vcall->_numTargets; i++)
	{
		OutputVirtualCall(vcall->_caller, vcall->_targets[i]);
	}
}

/// <summary>
/// Resolves the virtual call.
/// </summary>
/// <param name="target">The target.</param>
/// <param name="vcallList">The vcall list.</param>
static void ResolveVirtualCall(ADDRINT target, VirtualCall *vcallList)
{
	if (vcallList->_numTargets > 0x40) // Only allow a max of 64 Virtual Calls to be resolved (avoids building a massive list when JIT code is constantly being cycled)
		return;

	// Check that we haven't already resolved this target for this caller
	for (int i = 0; i < vcallList->_numTargets; i++)
	{
		if (vcallList->_targets[i] == target)
			return;
	}

	vcallList->_numTargets++;
	vcallList->_targets = (ADDRINT *)realloc(vcallList->_targets, vcallList->_numTargets * sizeof(void *));
	vcallList->_targets[vcallList->_numTargets - 1] = target;
	resolvedCount++; // Increment global counter

	if (!KnobDeferOutput.Value())
		OutputVirtualCall(vcallList->_caller, target);
}

/// <summary>
/// Outputs the marked BBL as scrupt.
/// </summary>
/// <param name="bb">The bb.</param>
static void OutputMarkedBbl(BblTrace *bb)
{
	if (!bb->_marked)
		return;

	std::ostringstream ss;
	ss << setfill('0');

	ss << "mark(0x" << setw(8) << hex << (bb->_address - imgBaseAddr) << ")";
	if (KnobVerbose.Value())
	{
		ModuleEntry *mod = GetModuleEntry(bb->_address);
		ss << "\t# " << (mod == 0 ? string("__unk__") : mod->_name) << "!" << RTN_FindNameByAddress(bb->_address);
	}
	ss << endl;

	bbcount++;

	output(ss.str());
}

/// <summary>
/// Logs the BBL execution.
/// </summary>
/// <param name="bb">The bb.</param>
static void LogBbl(BblTrace *bb)
{
	if (bb->_marked)
		return;

	bb->_marked = true;
	if (!KnobDeferOutput.Value())
		OutputMarkedBbl(bb);
}

/// <summary>
/// Trace instrumentation callback.
/// </summary>
/// <param name="trace">The trace.</param>
/// <param name="v">The v.</param>
static void PrintTrace(TRACE trace, VOID *v)
{
	if (!FilterTrace(trace))
		return;

	// Visit every basic block  in the trace
	for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
	{
		if (!KnobNoTrace.Value())
		{
			BblTrace * bb = new BblTrace;
			bb->_address = BBL_Address(bbl);
			bb->_marked = false;
			bb->_next = bblTraceList;
			bblTraceList = bb;

			if (bbl == TRACE_BblHead(trace))
			{
				bb->_marked = true;
				if (!KnobDeferOutput.Value())
					OutputMarkedBbl(bb);
			}
			else
			{
				BBL_InsertCall(bbl, IPOINT_BEFORE, AFUNPTR(LogBbl), IARG_PTR, bb, IARG_END);
			}
		}

		// Resolve virtual calls within the module
		if (!KnobNoResolveVirtualCalls.Value())
		{
			// Forward pass over all instructions in bbl
			for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
			{
				if (INS_IsCall(ins) && INS_IsIndirectBranchOrCall(ins))
				{
					ADDRINT caller = INS_Address(ins);
					VirtualCall *vcallList = new VirtualCall;
					vcallList->_caller = caller;
					vcallList->_next = virtualCallList;
					vcallList->_targets = 0;
					vcallList->_numTargets = 0;

					virtualCallList = vcallList;

					// Instrument all the Indirect Calls to Resolve Virtual Calls
					INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ResolveVirtualCall), IARG_BRANCH_TARGET_ADDR, IARG_PTR, vcallList, IARG_END);
				}
			}
		}
	}
}

/// <summary>
/// Image instrumentation callback. This is called when a module is loaded.
/// </summary>
/// <param name="img">The img.</param>
/// <param name="v">The v argument (optional).</param>
static VOID ImageLoad(IMG img, VOID *v)
{
	string name = string(IMG_Name(img));

	if (KnobVerbose.Value())
	{
		std::ostringstream ss;
		ss << setfill('0');
		ss << "# Loaded " << name << "\t" << setw(8) << hex << IMG_LowAddress(img) << " - " << setw(8) << hex << IMG_HighAddress(img) << endl << flush;
		output(ss.str());
	}

	string imgName = ToLower(FilenameWithoutExtension(StripPath(name)));

	if (!GetModuleEntry(IMG_LowAddress(img)))
	{
		// Save the module info for later output
		ModuleEntry *entry = new ModuleEntry;
		entry->_next = moduleList;
		entry->_start = IMG_LowAddress(img);
		entry->_end = IMG_HighAddress(img);
		entry->_name = imgName;

		moduleList = entry;
	}

	if (!module.empty() && imgName.compare(module) == 0)
	{
		imgBaseAddr = IMG_LowAddress(img);
		imgEndAddr = IMG_HighAddress(img);

		// Trace Instrument
		TRACE_AddInstrumentFunction(PrintTrace, 0);
	}

	InvalidateTraceExternal();
}

/// <summary>
/// Prints deferred output.
/// </summary>
static void DeferredOutput()
{
	// Output the VirtualCall List	
	output("# virtualCallList\n");
	while (virtualCallList != NULL)
	{
		OutputResolvedVirtualCall(virtualCallList);
		virtualCallList = virtualCallList->_next;
	}

	// Output the BblTrace List
	output("# bblTraceList");
	while (bblTraceList != NULL)
	{
		OutputMarkedBbl(bblTraceList);
		bblTraceList = bblTraceList->_next;
	}
}

/// <summary>
/// This function is called when the application exits.
/// </summary>
static VOID Fini(INT32 code, VOID *v)
{
	if (KnobDeferOutput.Value()) // if not live, display info on process exit
		DeferredOutput();
	
	if (KnobVerbose.Value())
	{
		std::ostringstream ss;
		ss << setfill('0');
		
		ss << "#======================================" << endl;
		if (!KnobNoTrace.Value())
			ss << "# " << setw(8) << hex << bbcount << "  -  Unique Basic Blocks" << endl;
		if (!KnobNoResolveVirtualCalls.Value())
			ss << "# " << setw(8) << hex << resolvedCount << "  -  Virtual Calls Resolved" << endl;
		ss << "#======================================" << endl << flush;
		
		output(ss.str());
	}

	out->flush();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

/// <summary>
/// Outputs the Usage summary for this pintool.
/// </summary>
/// <returns>-1</returns>
static INT32 Usage()
{
	cerr << "Usage:" << endl << "\tThis tool extracts information from a process as it executes to augment static analysis in IDA Pro" << endl;
	cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
	return -1;
}


/// <summary>
/// Writes the script header used to import the information into IDA.
/// </summary>
static void WriteScriptHeader()
{
	std::ostringstream ss;
	ss << setfill('0');

	ss << "#" << endl;
	ss << "# Paul Mehta 2012-10-10		paul@paulmehta.com" << endl;
	ss << "# IDA Pro (6.3) script file (Created using Ablation)" << endl;
	ss << "# " << endl;
	ss << "# Imports the results from Ablation and color's the imported BasicBlocks." << endl;
	ss << "# " << endl;
	ss << "#############<ScriptHeader>#############" << endl;
	ss << "from idaapi import *" << endl;
	ss << "from idautils import *" << endl;
	ss << "" << endl;
	ss << "color = 0xFFFFFF" << endl;
	ss << "moduleBase = FirstSeg() & 0xFFFF0000" << endl;
	ss << "" << endl;
	ss << "def ColorInstruction(instructionEA, col):" << endl;
	ss << "	SetColor(instructionEA, 1, col)" << endl;
	ss << "" << endl;
	ss << "def ColorChunk(chunkEA, col):" << endl;
	ss << "" << endl;
	ss << "	instructionEA = 0" << endl;
	ss << "	end = GetFchunkAttr(chunkEA, FUNCATTR_END)" << endl;
	ss << "	ea = GetFchunkAttr(chunkEA, FUNCATTR_START)" << endl;
	ss << "	" << endl;
	ss << "	while(ea != BADADDR):" << endl;
	ss << "		ColorInstruction(ea, col)" << endl;
	ss << "		ea = NextHead(ea, end)" << endl;
	ss << "	" << endl;
	ss << "def ColorFunctionInstructions(functionEA, col):" << endl;
	ss << "	ea = FirstFuncFchunk(functionEA)" << endl;
	ss << "" << endl;
	ss << "	while (ea != BADADDR):" << endl;
	ss << "		ColorChunk(ea, col)" << endl;
	ss << "		ea = NextFuncFchunk(functionEA, ea)" << endl;
	ss << "	" << endl;
	ss << "def ColorFunction(functionEA, col):" << endl;
	ss << "	SetColor(functionEA, 2, col)" << endl;
	ss << "	" << endl;
	ss << "def ColorBasicBlock(basicBlockEA, col):" << endl;
	ss << "	instr = basicBlockEA" << endl;
	ss << "	end = GetFchunkAttr(basicBlockEA, FUNCATTR_END)" << endl;
	ss << "	end = PrevHead(end, basicBlockEA)" << endl;
	ss << "	" << endl;
	ss << "	while (Rfirst0(instr) == BADADDR and instr != end):" << endl;
	ss << "		ColorInstruction(instr, col)	" << endl;
	ss << "		instr = Rfirst(instr)" << endl;
	ss << "		if(instr == BADADDR):" << endl;
	ss << "			break" << endl;
	ss << "	if(instr != BADADDR):" << endl;
	ss << "		ColorInstruction(instr, col)" << endl;
	ss << "" << endl;
	ss << "	print \"%X 	Marked\" % (basicBlockEA)" << endl;
	ss << "" << endl;
	ss << "def mark(basicBlockEA):" << endl;
	ss << "	basicBlockEA = moduleBase + basicBlockEA" << endl;
	ss << "	if(GetFunctionAttr(basicBlockEA, FUNCATTR_START) ==  basicBlockEA):" << endl;
	ss << "		ColorFunction(GetFunctionAttr(basicBlockEA, FUNCATTR_START), color)" << endl;
	ss << "		ColorFunctionInstructions(GetFunctionAttr(basicBlockEA, FUNCATTR_START), 0xFFFFFF)	" << endl;
	ss << "	ColorBasicBlock(basicBlockEA, color)" << endl;
	ss << "" << endl;
	ss << "def GetDemangledName(ea):" << endl;
	ss << "	name = Name(ea)" << endl;
	ss << "	" << endl;
	ss << "	if(name.find(\"@@\") > 0):" << endl;
	ss << "		name = Demangle(name, INF_SHORT_DN)" << endl;
	ss << "	return name" << endl;
	ss << "" << endl;
	ss << "def xstr(s):" << endl;
	ss << "	if s is None:" << endl;
	ss << "		return str(\"\")" << endl;
	ss << "	else:" << endl;
	ss << "		return str(s)" << endl;
	ss << "        " << endl;
	ss << "def InsertXRefComment(address, comment):" << endl;
	ss << "	existing = xstr(GetCommentEx(address, 0))" << endl;
	ss << "	if(len(existing) > 0 and not comment in existing):" << endl;
	ss << "		comment = \"%s\\n%s\" %(comment, existing)" << endl;
	ss << "	MakeComm(address, comment)" << endl;
	ss << "	" << endl;
	ss << "def createXRef(caller, target):" << endl;
	ss << "	caller += moduleBase" << endl;
	ss << "	target += moduleBase" << endl;
	ss << "	comment = \"%X   %s\" %(target, GetDemangledName(target))" << endl;
	ss << "	commentFrom = \"%X   %s\" % (caller, GetDemangledName(caller))" << endl;
	ss << "	InsertXRefComment(caller, comment)" << endl;
	ss << "	AddCodeXref(caller, target, fl_CN)	" << endl;
	ss << "	print \"XRef Created: %s  =>  %s\\n\" % (commentFrom, comment)" << endl;
	ss << "" << endl;
	ss << "def createXRefExternal(caller, comment):" << endl;
	ss << "	caller += moduleBase" << endl;
	ss << "	#commentFrom = \"%X   %s\" % (caller, GetDemangledName(caller))	" << endl;
	ss << "	commentFrom = \"%X   %s\" % (caller, Demangle(Name(caller), INF_SHORT_DN))" << endl;
	ss << "	InsertXRefComment(caller, comment)" << endl;
	ss << "	print \"External XRef Created: %s  =>  %s\\n\" % (commentFrom, comment)" << endl;
	ss << "" << endl;
	ss << "" << endl;
	ss << "print \"Using Module Base: %X\" % (moduleBase)" << endl;
	ss << "" << endl;
	ss << "color = " << KnobTraceColor.Value() << endl;
	ss << "" << endl;
	ss << "#############</ScriptHeader>#############" << endl;

	ss << endl;

	output(ss.str());
	out->flush();
}

/// <summary>
/// Checks if file with the specified name exists.
/// </summary>
/// <param name="name">The filename.</param>
/// <returns>true if file exists, false otherwise</returns>
bool FileExists(string name)
{
	if (FILE *file = fopen(name.c_str(), "r"))
	{
		fclose(file);
		return true;
	}
	return false;
}

/// <summary>
/// Initialize some basic parameters.
/// </summary>
static bool Initialize(int argc, char *argv[])
{
	std::ostringstream ss;

	module = ToLower(KnobModule.Value());

	if (module.empty())
		return false;

	fileout = KnobOutputFile.Value();

	if (fileout.empty() || fileout.compare(".") == 0)
	{
		fileout = module + ".ablation.py";

		//if (FileExists(fileout) && !KnobAppend.Value())
		//{
			ss.str("");
			ss << setfill('0');

			time_t t = time(0);   // get time now

			struct tm * now = localtime(&t);

			ss << module << ".ablation."
				<< (now->tm_year + 1900) << '-'
				<< setw(2) << (now->tm_mon + 1) << '-'
				<< setw(2) << now->tm_mday << "."
				<< setw(2) << now->tm_hour << 'h'
				<< setw(2) << now->tm_min
				<< ".py";

			fileout = ss.str();
		//}
	}

	out = &cout;
	if (!fileout.empty() && fileout.compare("console") != 0 && fileout.compare("cout") != 0)
	{
		out = new std::ofstream(fileout.c_str(),  (KnobAppend.Value() ? fstream::out : fstream::out | fstream::app));
	}

	//ss << setfill('0');
	ss.str("");

	if (!KnobAppend.Value())
		WriteScriptHeader();

	if (KnobVerbose.Value())
	{
		ss << "# PID: " << _getpid() << endl;
		ss << "# Granularity: " << "Basic Blocks" << endl;
		ss << "# Target Module: " << (module.empty() ? "*" : module) << endl;
		ss << "# Trace: " << boolalpha << !KnobNoTrace.Value() << endl;
		ss << "# Resolve Virtual Calls: " << boolalpha << !KnobNoResolveVirtualCalls.Value() << endl;
		ss << "# Verbose: " << boolalpha << KnobVerbose.Value() << endl;
		ss << "# Symbols Loaded: " << boolalpha << !KnobNoSymbols.Value() << endl;

		output(ss.str());
	}
	
	out->flush();

	return true;
}

/// <summary>
/// Pin calls this function every time a new img is unloaded 
/// You can't instrument an image that is about to be unloaded
/// </summary>
VOID ImageUnload(IMG img, VOID *v)
{
	std::ostringstream ss;

	if (KnobVerbose.Value())
	{
		ss << "# Unloading " << IMG_Name(img).c_str() << endl;
		output(ss.str());
	}

	ModuleEntry *prev = NULL;

	string imgName = ToLower(FilenameWithoutExtension(StripPath(string(IMG_Name(img)))));
	for (ModuleEntry *mod = moduleList; mod != 0; mod = mod->_next)
	{
		if (mod->_start == IMG_LowAddress(img) &&
			mod->_end == IMG_HighAddress(img) &&
			mod->_name == IMG_Name(img))
		{

			if (!module.empty() && imgName.compare(module) == 0)
			{
				if (KnobDeferOutput.Value()) // if not live, display info on process exit
					DeferredOutput();

				// free the VirtualCall list
				output("# Freeing virtualCallList\n");
				while (virtualCallList != NULL)
				{
					if (virtualCallList->_targets)
						free(virtualCallList->_targets); // free call targets

					VirtualCall *next = virtualCallList->_next;
					delete virtualCallList; // free the VirtualCall
					virtualCallList = next;
				}

				// free the VirtualCall list
				output("# Freeing bblTraceList\n");
				while (bblTraceList != NULL)
				{
					BblTrace *next = bblTraceList->_next;
					delete bblTraceList;
					bblTraceList = next;
				}
			}

			if (prev != NULL)
			{
				prev->_next = mod->_next;
				delete mod;
				return;
			}
		}

		prev = mod;
	}

	out->flush();
}

/// <summary>
/// Main.
/// </summary>
int main(int argc, char * argv[])
{
	// Load symbols
	if (!KnobNoSymbols.Value())
	{
		PIN_InitSymbolsAlt(DEBUG_OR_EXPORT_SYMBOLS);
	}

	// Initilize pintool
	if (PIN_Init(argc, argv))
		return Usage();

	// Initialize Ablation
	if (!Initialize(argc, argv))
		return -1;

	// Watch for specified module
	IMG_AddInstrumentFunction(ImageLoad, 0);

	// Register ImageUnload to be called when an image is unloaded
	IMG_AddUnloadFunction(ImageUnload, 0);

	// Register Fini to be called when the application exits
	PIN_AddFiniFunction(Fini, 0);

	// Start the program, never returns
	PIN_StartProgram();

	return 0;
}

