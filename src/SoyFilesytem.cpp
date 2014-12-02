#include "SoyFilesytem.h"
#include "SoyDebug.h"


SoyFilesystem::Timestamp::Timestamp()
{
#if defined(TARGET_OSX)
	memset( &mTime, 0, sizeof(mTime) );
#endif
}


bool SoyFilesystem::Timestamp::IsValid() const
{
	return (*this) != Timestamp();
}

bool SoyFilesystem::Timestamp::operator==(const Timestamp& that) const
{
	auto cmp = memcmp( &this->mTime, &that.mTime, sizeof(mTime) );
	return cmp == 0;
}


SoyFilesystem::File::File(Path Path,std::string Filename)
{
	//	extract path from filename...
	mName += Path.GetFullPath();
	mName += Filename;
}


SoyFilesystem::File::File(std::string Filename) :
	mName	( Filename )
{
}

std::string SoyFilesystem::File::GetFullPathFilename() const 
{
	auto Path = GetPath();
	std::stringstream FullPathFilename;
	FullPathFilename << Path.GetFullPath();
	FullPathFilename << mName;
	return FullPathFilename.str();
}

SoyFilesystem::Path SoyFilesystem::File::GetPath() const
{
	//	todo
	SoyFilesystem::Path Path("./");
	return Path;
}


SoyFilesystem::Timestamp SoyFilesystem::File::GetModified()
{
#if defined(TARGET_OSX)
	struct stat attrib;			// create a file attribute structure
	auto Filename = GetFullPathFilename();
	auto Result = stat( Filename.c_str(), &attrib);		// get the attributes of afile.txt
	if ( Result != 0 )
	{
		std::Debug << "Error getting filestamp(stat) of " << Filename << ": " << Soy::Platform::GetLastErrorString() << std::endl;
		return Timestamp();
	}
	
	SoyFilesystem::Timestamp Time;
				// create a time structure
	if ( !gmtime_r( &attrib.st_mtime, &Time.mTime ) )
	{
		std::Debug << "Error getting filestamp(gmtime) of " << Filename << ": " << Soy::Platform::GetLastErrorString() << std::endl;
		return Timestamp();
	}
		
	return Time;
#else
	return Timestamp();
#endif
}



SoyFilesystem::Path::Path(std::string Path) :
	mName	( Path )
{
	
}


std::string SoyFilesystem::Path::GetFullPath() const
{
	//	todo
	return mName;
}


