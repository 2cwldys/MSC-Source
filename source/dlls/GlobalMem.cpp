/*
	GlobalMem.cpp - Shared global memory control
*/

#include "MSDLLHeaders.h"
#include "Global.h"
#include "logfile.h"


#ifdef VALVE_DLL
	//#define TRACK_MEMORY		//Deterimines whether all memory allocations should be catalogued and dumped to file

	#ifdef TRACK_MEMORY
		memalloc_t *Allocations[500000];
		int alloctotal = 0;
		int allocid = 0;
		int allochighest = 0;
		memalloc_t MemAlloc;
		bool FileDetails = false;
		int AllocationStack = 0;
		void DisableAllocateTrace( ) { AllocationStack++; }
		void EnableAllocateTrace( ) { AllocationStack--; }
	#endif
#endif

#define MS_FATAL_ERROR_MEM( Message ) MessageBox( NULL, Message, "Memory Error", MB_OK );
extern bool g_MemWarningActive;

//Debug memory testing
//This is only enabled when DEV_BUILD is defined

void *operator new( size_t size, const char *pszSourceFile, int LineNum )
{
	#ifdef TRACK_MEMORY
		FileDetails = true;

		MemAlloc.SourceFile = pszSourceFile;
		MemAlloc.LineNum = LineNum;
	#endif

	return operator new( size );
}
void *operator new( size_t size )
{
	try
	{
	void *pAddr = malloc( size );
	if( !pAddr )
	{
		MS_FATAL_ERROR_MEM( "Out of Memory.  Couldn't Allocate New Block" );
		return NULL;
	}

	memset( pAddr, 0, size );	//Dogg: Initialize all new memory

	#ifdef TRACK_MEMORY
		if( !AllocationStack )
		{
			if( !FileDetails )
			{
				MemAlloc.SourceFile = "No file";
				MemAlloc.LineNum = 0;
			}
			MemAlloc.pAddr = pAddr;
			MemAlloc.Used = true;

			//Use breakpoints here to track down leaking memory
			//Have them output to file, then trace them here by
			//their ID
			if( allocid == 128027 )
				int stop = 0;
			
			MemAlloc.Index = allocid++;
			if( MemAlloc.Index > allochighest ) allochighest = MemAlloc.Index;
			if( allocid >= ARRAYSIZE(Allocations) )
			{
				logfile << "Error: Alloc New Memory: Allocations exceed max debug size (" << ARRAYSIZE(Allocations) << ")" << endl;
				int stop = 0;
				exit( 1 );
				return NULL;
			}

			DisableAllocateTrace( );
			Allocations[alloctotal++] = new memalloc_t( MemAlloc );
			EnableAllocateTrace( );
		}
		FileDetails = false;
	#endif

	MemMgr::NewAllocation( pAddr, size );

	return pAddr;
	}
	catch( ... ) 
	{
		MS_FATAL_ERROR_MEM( "Unhandled Exception While Allocating Memory" )
	}
	return NULL;
}
void operator delete( void *ptr, const char *pszSourceFile, int LineNum ) { delete ptr; }

//#ifdef TRACK_MEMORY
void operator delete( void *ptr )
{
	try
	{

	#ifdef TRACK_MEMORY
		if( !AllocationStack )
		{
			bool found = false;
			foreach( i, alloctotal )
				if( Allocations[i]->pAddr == ptr )
				{
					found = true;
					DisableAllocateTrace( );


					//Use this to track a certain memory deletion
					if( Allocations[i]->Index == 127750 )
						int stop = 0;

					delete Allocations[i];
					if( i < alloctotal-1 )
						memcpy( &Allocations[i], &Allocations[i+1], sizeof(void *) * (alloctotal-i-1) );
					alloctotal--;
					EnableAllocateTrace( );
					break;
				}

			if( !found )
			{
				logfile << "Delete Memory: Tried to delete memory that wasn't allocated (" << Allocations[i]->SourceFile.c_str() << ":" << Allocations[i]->LineNum + ")" << endl;
				exit( 1 );
				return;
			}
		}
	#endif

	MemMgr::NewDeallocation( ptr );

	free( ptr );
	}
	catch( ... )
	{
		MS_FATAL_ERROR_MEM( "Unhandled Exception While Deallocating Memory" )
	}
}

//#endif

void LogMemoryUsage( msstring_ref Title )
{
	#ifdef TRACK_MEMORY
		logfile << Title << endl;
		logfile << "[Current Memory Allocations: " << alloctotal << "][Highest Ever: " << allochighest << "]" << endl;
		foreach( i, alloctotal )
		{
			//if( Allocations[i]->Index == 124117 )
			//	int stop = 0;

			logfile << "[Unfreed #" << i << "][" << Allocations[i]->Index << "] "  << Allocations[i]->SourceFile.c_str() << " : " << Allocations[i]->LineNum << endl;
		}
	#endif
}