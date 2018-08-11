#pragma once

#include "SoyThread.h"

class SoyApp
{
public:
	SoyApp()					{}
	virtual ~SoyApp()			{}

	virtual bool	Init()		{	return true;	}
	virtual bool	Update()	{	return true;	}
	virtual void	Exit()		{}
};

namespace Soy
{
	namespace Platform
	{
		class TConsoleApp;
	}
};

class Soy::Platform::TConsoleApp
{
public:
	void				Exit();
	void				WaitForExit();
private:
#if defined(TARGET_WINDOWS)
	static BOOL WINAPI	ConsoleHandler(DWORD dwType);
#endif
	
private:
	SoyWorkerDummy		mWorker;
};



class TBitReader
{
public:
	TBitReader(const ArrayBridge<char>& Data) :
		mData	( Data ),
		mBitPos	( 0 )
	{
	}
	
	bool						Eof() const			{	return BitPosition() >= (mData.GetDataSize()*8);	}
	bool						Read(int& Data,int BitCount);
	bool						Read(uint64& Data,int BitCount);
	bool						Read(uint8& Data,int BitCount);
	int							Read(int BitCount)					{	int Data;	return Read( Data, BitCount) ? Data : -1;	}
	size_t						BitPosition() const					{	return mBitPos;	}
	size_t						BytesRead() const
	{
		auto RoundUpBits = (8 - (mBitPos % 8)) % 8;
		return (mBitPos+RoundUpBits) / 8;
	}

	template<int BYTES,typename STORAGE>
	bool						ReadBytes(STORAGE& Data,int BitCount);

private:
	const ArrayBridge<char>&	mData;
	unsigned int				mBitPos;	//	current bit-to-read/write-pos (the tail)
};

class TBitWriter
{
public:
	TBitWriter(ArrayBridge<char>& Data) :
		mData	( Data ),
		mBitPos	( 0 )
	{
	}
	
	unsigned int				BitPosition() const					{	return mBitPos;	}
	
	void						Write(uint8 Data,int BitCount);
	void						Write(uint16 Data,int BitCount);
	void						Write(uint32 Data,int BitCount);
	void						Write(uint64 Data,int BitCount);
	void						WriteBit(int Bit);

	template<int BYTES,typename STORAGE>
	void						WriteBytes(STORAGE Data,int BitCount);

private:
	ArrayBridge<char>&	mData;
	unsigned int		mBitPos;	//	current bit-to-read/write-pos (the tail)
};
