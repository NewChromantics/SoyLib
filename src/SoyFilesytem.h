#pragma once

#include "SoyTypes.h"

#if defined(TARGET_OSX)
	#include <sys/stat.h>
	#include <unistd.h>
	#include <time.h>
#endif


namespace SoyFilesystem
{
	class Timestamp;	//	file timestamp
	class File;			//
	class Path;			//	directory
};



class SoyFilesystem::Path
{
public:
	Path(std::string Path);
	
	bool		IsValid()		{	return !mName.empty();	}
	bool		Exists();
	bool		IsFullPath() const;
	std::string	GetFullPath() const;
	std::string	GetPath() const;
	
private:
	std::string	mName;
};



class SoyFilesystem::File
{
public:
	File(Path Path,std::string Filename);
	File(std::string Filename="");
	
	std::string	GetFilename() const	{	return mName;	}
	std::string	GetFullPathFilename() const;
	bool		IsValid()			{	return !mName.empty();	}
	bool		Exists();
	Timestamp	GetModified();
	Path		GetPath() const;
	bool		HasAbsolutePath() const;

	inline bool	operator==(const File& that) const	{	return this->GetFullPathFilename() == that.GetFullPathFilename();	}
	
private:
	std::string	mName;
};




class SoyFilesystem::Timestamp
{
public:
	Timestamp();
	
	bool		IsValid() const;
	
	bool		operator==(const Timestamp& that) const;
	bool		operator!=(const Timestamp& that) const		{	return !(*this == that);	}
	
public:
#if defined(TARGET_OSX)
	struct tm	mTime;
#endif
};

